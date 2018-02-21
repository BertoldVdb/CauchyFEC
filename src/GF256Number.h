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

#ifndef GF256NUMBER_H_
#define GF256NUMBER_H_

#include <stdexcept>
#include <cstdint>

template <uint16_t P = 0x18b, uint8_t G = 0x87> class GF256Number {
public:
    inline GF256Number(const GF256Number<P>& value) {
        value_ = value.value_;
    }

    inline GF256Number(uint8_t value) {
        value_ = value;
    }

    inline GF256Number() {
        value_ = 0;
    }

    static void init() {
        buildTables();
    }

    inline void operator=(const GF256Number<P>& value) {
        value_ = value.value_;
    }

    inline void operator=(uint8_t value) {
        value_ = value;
    }

    inline GF256Number<P> operator+(const GF256Number<P>& b) const {
        return GF256Number(gfAddSub(value_, b.value_));
    }

    inline void operator+=(const GF256Number<P>& b) {
        value_ = gfAddSub(value_, b.value_);
    }

    inline GF256Number<P> operator-(const GF256Number<P>& b) const {
        return GF256Number<P>(gfAddSub(value_, b.value_));
    }

    inline void operator-=(const GF256Number<P>& b) {
        value_ = gfAddSub(value_, b.value_);
    }

    inline GF256Number<P> operator*(const GF256Number<P>& b) const {
        return GF256Number(gfMultTable(value_, b.value_));
    }

    inline void operator*=(const GF256Number<P>& b) {
        value_ = gfMultTable(value_, b.value_);
    }

    inline GF256Number<P> operator/(const GF256Number<P>& b) const {
        return GF256Number(gfDivTable(value_, b.value_));
    }

    inline void operator/=(const GF256Number<P>& b) {
        value_ = gfDivTable(value_, b.value_);
    }

    inline uint8_t value() const {
        return value_;
    }

    inline operator uint8_t() const {
        return value_;
    }

    inline bool operator==(const GF256Number<P>& b) const {
        return value_ == b.value_;
    }

    inline bool operator!=(const GF256Number<P>& b) const {
        return !operator==(b);
    }

private:
    uint8_t value_;

    inline uint8_t gfAddSub(uint8_t a, uint8_t b) const {
        return a^b;
    }

    inline uint8_t gfMultTable(uint8_t a, uint8_t b) const {
        if(a == 0 || b == 0) {
            return 0;
        }
        return expTable_[logTable_[a] + logTable_[b] + 255];
    }

    inline uint8_t gfDivTable(uint8_t a, uint8_t b) const {
        if(b == 0) {
            throw std::invalid_argument("Division by 0");
        }
        if(a == 0) {
            return 0;
        }
        return expTable_[logTable_[a] - logTable_[b] + 255];
    }

    static uint8_t gfMultSlow(uint8_t a, uint8_t b) {
        uint16_t result = 0;

        /* Multiply */
        for(unsigned int i = 0; i < 8; i++) {
            if(b & (1 << i)) {
                result ^= a << i;
            }
        }

        /* Reduce */
        for(unsigned int i = 15; i >= 8; i--) {
            if(result & (1 << i)) {
                result ^= P << (i-8);
            }
        }

        return result;
    }


    static void buildTables() {
        if(tableBuilt_) return;

        /* a^0 == 1 */
        expTable_[0] = 1;
        logTable_[1] = 0;

        /* Log(0) has no result */
        logTable_[0] = 0;

        /* Calculate other entries */
        for(unsigned int i=1; i<256; i++) {
            uint8_t tmp = gfMultSlow(expTable_[i-1], G);

            /* Fill in exponent table */
            expTable_[i] = tmp;
            expTable_[i + 255] = tmp;
            expTable_[i + 255 * 2] = tmp;

            /* Fill in log table */
            logTable_[tmp] = i;
        }

        tableBuilt_  = true;
    }

    /*
     * The exponent table is larger than needed, but this saves somewhat
     * expensive mod 255 operations.
     */
    static uint8_t expTable_[256+255+255];
    static uint8_t logTable_[256];
    static bool tableBuilt_;
};

template <uint16_t P, uint8_t G> uint8_t GF256Number<P, G>::expTable_[256+255+255];
template <uint16_t P, uint8_t G> uint8_t GF256Number<P, G>::logTable_[256];
template <uint16_t P, uint8_t G> bool    GF256Number<P, G>::tableBuilt_ = false;




#endif /* GF256NUMBER_H_ */
