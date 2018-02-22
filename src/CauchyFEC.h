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

#include <vector>
#include <memory>

#ifndef CAUCHYFEC_H_
#define CAUCHYFEC_H_

#define CAUCHYFEC_H_EXPORT_FUNCTION __attribute__((visibility("default")))

class CauchyFEC {
public:
    static CAUCHYFEC_H_EXPORT_FUNCTION void init();
    CAUCHYFEC_H_EXPORT_FUNCTION CauchyFEC();
    CAUCHYFEC_H_EXPORT_FUNCTION ~CauchyFEC();

    void CAUCHYFEC_H_EXPORT_FUNCTION reset(bool encode, unsigned int numberOfSourcePackets = 0);
    void CAUCHYFEC_H_EXPORT_FUNCTION operator<<(const std::vector<uint8_t>& sourcePacket);
    void CAUCHYFEC_H_EXPORT_FUNCTION operator<<(const std::vector<std::vector<uint8_t>>& sourcePackets);
    unsigned int CAUCHYFEC_H_EXPORT_FUNCTION requestPackets(std::vector<std::vector<uint8_t>>& outputPackets, unsigned int numPackets = 1);
    bool CAUCHYFEC_H_EXPORT_FUNCTION operator>>(std::vector<uint8_t>& outputPackets);
    bool CAUCHYFEC_H_EXPORT_FUNCTION operator>>(std::vector<std::vector<uint8_t>>& outputPackets);

private:
    class impl;
    std::unique_ptr<impl> impl_;
};

#endif /* CAUCHYFEC_H_ */
