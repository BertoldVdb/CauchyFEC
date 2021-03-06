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

#include "CauchyFECImpl.h"

void CauchyFEC::init() {
    CauchyFEC::impl::init();
}

CauchyFEC::CauchyFEC():
    impl_(new impl()) {
}

void CauchyFEC::reset(bool encode, unsigned int numberOfSourcePackets) {
    impl_->reset(encode, numberOfSourcePackets);
}

void CauchyFEC::operator<<(const std::vector<uint8_t>& sourcePacket) {
    impl_->operator<<(sourcePacket);
}

void CauchyFEC::operator<<(const std::vector<std::vector<uint8_t>>& sourcePackets) {
    impl_->operator<<(sourcePackets);
}

unsigned int CauchyFEC::requestPackets(std::vector<std::vector<uint8_t>>& outputPackets, unsigned int numPackets) {
    return impl_->requestPackets(outputPackets, numPackets);
}

bool CauchyFEC::operator>>(std::vector<uint8_t>& outputPackets) {
    return impl_->operator>>(outputPackets);
}

bool CauchyFEC::operator>>(std::vector<std::vector<uint8_t>>& outputPackets) {
    return impl_->operator>>(outputPackets);
}

CauchyFEC::~CauchyFEC() = default;
