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

#ifndef MATRIX_H_
#define MATRIX_H_


#include <cstdint>
#include <stdexcept>

template <typename T> class Matrix {
public:
    Matrix (unsigned int rows, unsigned int cols):
        rows_(rows),
        cols_(cols) {

        if(cols) {
            /* Convert cols_ into a binary shift */
            shift_ = 8 * sizeof(unsigned int) - __builtin_clz(cols);

            /* Allocate the elements */
            data_ = new T[rows_ << shift_];
        } else {
            shift_ = 0;
            data_ = nullptr;
        }
    }

    Matrix<T>(Matrix<T>&& old) {
        doMove(old);
    }

    Matrix<T>(const Matrix<T>& old):
        Matrix(old.rows_, old.cols_) {

        for(unsigned int row=0; row<rows_; row++) {
            for(unsigned int col=0; col<cols_; col++) {
                operator()(row, col) = old(row, col);
            }
        }
    }

    void operator=(Matrix<T>&& old) {
        cleanup();
        doMove(old);
    }

    void setAllElements(T value) {
        for(unsigned int row=0; row<rows_; row++) {
            for(unsigned int col=0; col<cols_; col++) {
                operator()(row, col) = value;
            }
        }
    }

    ~Matrix() {
        cleanup();
    }

    Matrix<T> operator[](unsigned int row) {
        Matrix rowMatrix;

        rowMatrix.rows_ = 1;
        rowMatrix.cols_ = cols_;
        rowMatrix.shift_ = shift_;
        rowMatrix.data_ = &operator()(row, 0);

        return rowMatrix;
    }

    void identity(T value) {
        for(unsigned int row=0; row<rows_; row++) {
            for(unsigned int col=0; col<cols_; col++) {
                operator()(row, col) = (row == col)?value:T(0);
            }
        }
    }

    void swapRows(unsigned int a, unsigned int b, unsigned int startCol = 0) {
        for(unsigned int i=startCol; i<cols_; i++) {
            T tmp = operator ()(a, i);
            operator ()(a, i) = operator ()(b, i);
            operator ()(b, i) = tmp;
        }
    }

    inline T& operator()(unsigned int row, unsigned int col) const {
        T& value = data_[(row << shift_) | col];
        return value;
    }

    inline T& at(unsigned int row, unsigned int col) const {
        if(row >= rows_ || col >= cols_) {
            std::runtime_error("Matrix index out of bounds");
        }
        return operator()(row, col);
    }

    inline Matrix operator+ (const Matrix& b) const {
        Matrix result(rows_, cols_);
        addWork(true, *this, b, result);
        return std::move(result);
    }

    inline void operator+= (const Matrix& b) {
        addWork(true, *this, b, *this);
    }

    inline Matrix operator- (const Matrix& b) const {
        Matrix result(rows_, cols_);
        addWork(false, *this, b, result);
        return std::move(result);
    }

    inline void operator-= (const Matrix& b) {
        addWork(false, *this, b, *this);
    }

    inline Matrix operator* (const Matrix& b) const {
        Matrix result(rows_, b.cols_);
        multiplyWork(*this, b, result);
        return std::move(result);
    }

    inline void operator*= (const Matrix& b) {
        multiplyWork(*this, b, *this);
    }

    inline void multiplyPreallocated(const Matrix& b, Matrix& target) const {
        multiplyWork(*this, b, target);
    }

    inline unsigned int rows() const {
        return rows_;
    }

    inline unsigned int columns() const {
        return cols_;
    }

    Matrix ():
        iOwnData_(false) {
    }

    bool operator!=(const Matrix<T>& b) const {
        return !operator==(b);
    }

    bool operator==(const Matrix<T>& b) const {
        if(b.rows_ != rows_) return false;
        if(b.cols_ != cols_) return false;

        for(unsigned int i=0; i<rows_; i++) {
            for(unsigned int j=0; j<cols_; j++) {
                if(operator ()(i, j) != b(i, j)) {
                    return false;
                }
            }
        }

        return true;
    }

private:

    void addWork(bool add, const Matrix& a, const Matrix& b, Matrix& target) const {
        if(a.cols_ != b.cols_ || a.rows_ != b.rows_) {
            throw std::runtime_error("Matrix dimensions are mismatched");
        }
        if(a.cols_ != target.cols_ || a.rows_ != target.rows_) {
            throw std::runtime_error("Target is unsuited");
        }

        if(add) {
            for(unsigned int row = 0; row < a.rows_; row++) {
                for(unsigned int col = 0; col < a.cols_; col++) {
                    target(row, col) = a(row, col) + b(row, col);
                }
            }
        } else {
            for(unsigned int row = 0; row < a.rows_; row++) {
                for(unsigned int col = 0; col < a.cols_; col++) {
                    target(row, col) = a(row, col) - b(row, col);
                }
            }
        }
    }

    void multiplyWork(const Matrix& a, const Matrix& b, Matrix& target) const {
        /* Check dimensions of target buffer */
        if(a.cols_ != b.rows_) {
            throw std::runtime_error("Matrix dimensions are mismatched");
        }
        if(target.rows_ != a.rows_ || target.cols_ != b.cols_) {
            throw std::runtime_error("Target is unsuited");
        }

        T scratchpad[a.rows_][b.cols_];

        for(unsigned int row = 0; row < a.rows_; row++) {
            for(unsigned int col = 0; col < b.cols_; col++) {
                scratchpad[row][col] = 0;
                for(unsigned int mIndex = 0; mIndex < a.cols_; mIndex ++) {
                    scratchpad[row][col] += operator()(row, mIndex) * b(mIndex, col);
                }
            }
        }

        /* Move results */
        for(unsigned int row = 0; row < a.rows_; row++) {
            for(unsigned int col = 0; col < b.cols_; col++) {
                target(row, col) = std::move(scratchpad[row][col]);
            }
        }

    }

    void cleanup() {
        if(iOwnData_ && data_) {
            delete[] data_;
        }
        iOwnData_ = false;
    }

    void doMove(Matrix<T>& old) {
        iOwnData_ = old.iOwnData_;
        data_ = old.data_;

        rows_ = old.rows_;
        shift_ = old.shift_;
        cols_ = old.cols_;

        old.iOwnData_ = false;
        old.data_ = nullptr;
    }

    unsigned int rows_ = 0;
    unsigned int cols_ = 0;
    unsigned int shift_ = 0;
    T* data_ = nullptr;
    bool iOwnData_ = true;
};

#endif /* MATRIX_H_ */
