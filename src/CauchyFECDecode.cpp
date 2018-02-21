/*
 * Copyright (c) 2018, Bertold Van den Bergh (vandenbergh@bertold.org)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the author nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR DISTRIBUTOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "CauchyFEC.h"

bool CauchyFEC::decoderMatrixInverse(Matrix<RSGF256Number>& matrix) {
    if(matrix.rows() != matrix.columns()) {
        throw std::runtime_error("Matrix not square");
    }

    Matrix<RSGF256Number> inverse(matrix.rows(), matrix.columns());
    inverse.identity(1);

    for(unsigned int pIndex = 0; pIndex < matrix.columns(); pIndex++) {
        /*
         * Since we perform integer calculations, selecting any non-zero pivot is fine.
         */
        RSGF256Number pivot = matrix(pIndex, pIndex);

        if(!pivot) {
            for(unsigned int pRow = pIndex + 1; pRow < matrix.rows(); pRow++) {
                if(matrix(pRow, pIndex)) {
                    /* Swap rows */
                    matrix.swapRows(pRow, pIndex);
                    inverse.swapRows(pRow, pIndex);
                    pivot = matrix(pIndex, pIndex);
                    break;
                }
            }

            if(!pivot) {
                /* This matrix is singular? */
                return false;
            }
        }

        /* Divide the line of the pivot */
        for(unsigned int col = pIndex; col < matrix.columns(); col++) {
            matrix(pIndex, col) /= pivot;
        }
        for(unsigned int col = 0; col < matrix.columns(); col++) {
            inverse(pIndex, col) /= pivot;
        }

        for(unsigned int row = 0; row < matrix.rows(); row++) {
            if(row == pIndex)
                continue;

            /* Make zeros by subtracting sub (pivot is 1 now) */
            RSGF256Number factor = matrix(row, pIndex);

            for(unsigned int col = pIndex; col < matrix.columns(); col++) {
                matrix(row, col) -= factor * matrix(pIndex, col);
            }
            for(unsigned int col = 0; col < matrix.columns(); col++) {
                inverse(row, col) -= factor * inverse(pIndex, col);
            }
        }
    }

    matrix = std::move(inverse);

    return true;
}

void CauchyFEC::decoderReset() {
    decoderWaitingFirstPacket_ = true;
    decoderOriginalPacketsReceived_ = 0;
    decoderPacketsReturned_ = 0;
    decoderStuck_ = false;
}

void CauchyFEC::decoderOperatorLL(const std::vector<uint8_t>& inputPacket) {
    /* No point in reading more packets if we won't be able to produce output */
    if(decoderStuck_) {
        return;
    }

    /* We don't allow 0 byte application level packets */
    if(inputPacket.size() <= 2) {
        return;
    }

    if(decoderWaitingFirstPacket_) {
        decoderWaitingFirstPacket_ = false;
        numSourcePackets_ = inputPacket[inputPacket.size() - 1] + 1;

        decoderPacketBuffer_.resize(numSourcePackets_);

        for(unsigned int i = 0; i < numSourcePackets_; i++) {
            decoderPacketBuffer_[i].clear();
        }
    } else {
        /* Same series? */
        if(numSourcePackets_ != (inputPacket[inputPacket.size() - 1] + 1U)) {
            return;
        }
    }

    uint8_t packetIndex = inputPacket[inputPacket.size() - 2];

    if(packetIndex < numSourcePackets_) {
        if(!decoderPacketBuffer_[packetIndex].size()) {
            decoderPacketBuffer_[packetIndex] = inputPacket;
            decoderPacketBuffer_[packetIndex].resize(inputPacket.size() - 2);
            decoderOriginalPacketsReceived_++;
        }
    } else {
        decoderPacketBuffer_.push_back(inputPacket);
    }
}

void CauchyFEC::decoderOperatorLL(const std::vector<std::vector<uint8_t>>& inputPackets) {
    for(auto& inputPacket: inputPackets) {
        decoderOperatorLL(inputPacket);
    }
}

bool CauchyFEC::decoderRun() {
    /* How many parity packets will we use */
    unsigned int parityPacketsNeeded = numSourcePackets_ - decoderOriginalPacketsReceived_;
    if(!parityPacketsNeeded) {
        return true;
    }

    if(parityPacketsNeeded > decoderPacketBuffer_.size() - numSourcePackets_) {
        /* This is doomed to fail from the start */
        return false;
    }

    /* Find N unique parity packets */
    uint8_t usedParityIndex = 0;
    uint8_t usedParity[parityPacketsNeeded];
    uint8_t usedParityPacketIndex[parityPacketsNeeded];
    uint64_t usedParityBitfield[4] = {0, 0, 0, 0};

    for(unsigned int i=numSourcePackets_; i<decoderPacketBuffer_.size(); i++) {
        auto& parity = decoderPacketBuffer_[i];
        uint8_t packetIndex = parity[parity.size() - 2];

        /* Did we already use this parity packet? */
        uint64_t mask = (uint64_t)1<<(packetIndex & 0x3F);
        if(!(usedParityBitfield[packetIndex >> 6] & mask)) {
            usedParityPacketIndex[usedParityIndex] = packetIndex;
            usedParity[usedParityIndex] = i;
            usedParityBitfield[packetIndex >> 6] |= mask;
            usedParityIndex++;

            if(usedParityIndex >= parityPacketsNeeded) {
                break;
            }
        }
    }

    if(usedParityIndex < parityPacketsNeeded) {
        return false;
    }

    /* Do they all have the same length? */
    unsigned int parityLength = decoderPacketBuffer_[usedParity[0]].size();

    for(unsigned int i=1; i<parityPacketsNeeded; i++) {
        if(decoderPacketBuffer_[usedParity[i]].size() != parityLength) {
            decoderStuck_ = true;
            return false;
        }
    }

    /* Strip metadata */
    parityLength -= 2;

    /* Build generator matrix for the desired packets */
    Matrix<RSGF256Number> generatorRectangularMatrix(parityPacketsNeeded, numSourcePackets_);
    Matrix<RSGF256Number> generatorSubMatrix(parityPacketsNeeded, parityPacketsNeeded);

    for(unsigned int i=0; i<parityPacketsNeeded; i++) {
        Matrix<RSGF256Number> generatorRow = generatorRectangularMatrix[i];
        getGeneratorRow(generatorRow, usedParityPacketIndex[i], numSourcePackets_);
    }

    /* Build parity message matrix */
    Matrix<RSGF256Number> parityMessage(parityPacketsNeeded, parityLength);
    for(unsigned int i=0; i<parityPacketsNeeded; i++) {
        auto& parity = decoderPacketBuffer_[usedParity[i]];
        for(unsigned int j=0; j<parityLength; j++) {
            parityMessage(i,j) = parity[j];
        }
    }

    /* Process known packets: if source packets are known we subtract them from the RHS
     * and remove the columns from the generator matrix. We should get a square generator
     * this way.
     */

    unsigned int generatorSubColumnIndex = 0;
    for(unsigned int i=0; i<numSourcePackets_; i++) {
        auto& goodPacket = decoderPacketBuffer_[i];

        if(goodPacket.size()) {
            /* Found, update parityMessage */
            for(unsigned int j=0; j<parityPacketsNeeded; j++) {
                for(unsigned int k=0; k<parityLength; k++) {
                    RSGF256Number c = 0;
                    if(k < goodPacket.size()) {
                        c = goodPacket[k];
                    } else if(k == parityLength - 2) {
                        c = goodPacket.size() >> 8;
                    } else if(k == parityLength - 1) {
                        c = goodPacket.size() & 0xFF;
                    }

                    parityMessage(j, k) -= generatorRectangularMatrix(j, i) * c;
                }
            }
        } else {
            /* Not found, calcluate it */
            for(unsigned int j=0; j<parityPacketsNeeded; j++) {
                generatorSubMatrix(j, generatorSubColumnIndex) =
                    generatorRectangularMatrix(j, i);
            }
            generatorSubColumnIndex++;
        }
    }

    /* Invert generator and decode */
    if(!decoderMatrixInverse(generatorSubMatrix)) {
        /* This should not happen, as the matrix is MDS */
        decoderStuck_ = true;
        return false;
    }

    auto decodedMessage = generatorSubMatrix * parityMessage;

    unsigned int decodedIndex = 0;
    for(unsigned int i=0; i<numSourcePackets_; i++) {
        if(!decoderPacketBuffer_[i].size()) {
            unsigned int packetSize = (decodedMessage(decodedIndex, parityLength-2) << 8) |
                                      (decodedMessage(decodedIndex, parityLength-1));


            if(packetSize > parityLength - 2) {
                /* What? This can't be decoded... */
                decoderStuck_ = true;
                return false;
            }

            auto& decodedPacket = decoderPacketBuffer_[i];
            decodedPacket.resize(packetSize);
            for(unsigned int j=0; j<packetSize; j++) {
                decodedPacket[j] = decodedMessage(decodedIndex, j);
            }

            decodedIndex++;
        }
    }

    return true;
}

unsigned int CauchyFEC::decoderRequestPackets(std::vector<std::vector<uint8_t>>& packets, unsigned int numPackets) {
    if(decoderStuck_) {
        return 0;
    }

    /* Can we actually deliver this many packets? */
    for(unsigned int packet = 0; packet < numPackets; packet++) {

        bool packetValid = false;
        if(decoderPacketsReturned_ < numSourcePackets_) {
            if(decoderPacketBuffer_[decoderPacketsReturned_].size()) {
                packetValid = true;
            } else {
                /* This source packet is missing. We need to attempt decoding... */
                if(decoderRun()) {
                    packetValid = true;
                }
            }
        }

        if(packetValid) {
            packets.push_back(decoderPacketBuffer_[decoderPacketsReturned_]);
            decoderPacketsReturned_++;
        } else {
            return packet;
        }

    }
    return numPackets;
}



