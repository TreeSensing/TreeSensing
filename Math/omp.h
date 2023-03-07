/* 
OMP (Orthogonal Matching Persuits) recovery algorithm for compressive sensing 
*/

#include <cstdio>
#include <cmath>
#include <sys/time.h>
#include <vector>
#include <algorithm>

#define NUM_ITERATION 10

namespace omp {

void decompress(float *A, float *y, float *x_hat, int m, int n);
int matMul(float *mat1, float *mat2, float *result, int m, int n, int q);
void transpose(float *mat1, float *mat2, int m, int n);
float innerMulColumn(float *mat1, float *vector, int nrows, int ncols, int C);
void matSUB(float *mat1, float *mat2, float *result, int m, int n);
int QR(float *A, float *Q, float *R, int n);
float innerMatColumnMAT(float *mat, int n, int C1, int C2);
int Union(int *vec, int newval);
void backsubstitution(float *R, float *y_Qt, float *x_hat, int n);
float norm_Col(float *A, int m, int n, int C);
int max_index(float *vector, int size);
float SNR(float *a, float *b, int Length);
float MSE(float *a, float *b, int Length);

static struct timeval tm1;
static inline void start() { gettimeofday(&tm1, NULL); }

static inline void stop() {
    struct timeval tm2;
    gettimeofday(&tm2, NULL);

    unsigned long long t =
        1000 * (tm2.tv_sec - tm1.tv_sec) + (tm2.tv_usec - tm1.tv_usec) / 1000;
    // printf("%llu ms\n", t);
}

int matMul(float *mat1, float *mat2, float *result, int m, int n, int q) {
    // Multiplication  for Mat1_(m*n) * Mat2_(n*k)
    //  M,N and K are dimentions of matrices
    //  Output is going to sit in the matrix that result represents here
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < q; j++) {
            float sum = 0;
            for (int k = 0; k < n; k++) {
                sum += mat1[i * n + k] * mat2[k * q + j];
            }
            result[i * q + j] = sum;
        }
    }
    return 0;
}

void transpose(float *mat1, float *mat2, int m, int n) {
    // transposes mat1_m*n which is input to mat2_n*m which is out
    //  M,N  dimentions of matrices
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            mat2[j * m + i] = mat1[i * n + j];
        }
    }
}

float innerMulColumn(float *mat, float *vec, int nrows, int ncols, int C) {
    // Returns inner porduct of a column vec of a and b matrix, colum number
    // 1,2,3,... d C is the column of interest
    float sum = 0;
    for (int i = 0; i < nrows; i++) {
        sum += mat[i * ncols + C] * vec[i];
    }
    return sum;  // The scalar value of inner product
}

int max_index(float *vec, int size) {
    int index = 0;
    float max = vec[index];
    for (int i = 0; i < size; i++) {
        if (vec[i] > max) {
            max = vec[i];
            index = i;
        }
    }
    return index;
}

int QR(float *A, float *Q, float *R, int n) {
    int i, j, K;
    for (i = 0; i < n; i++)  // Q = A;
        for (j = 0; j < n; j++) {
            *(Q + i * n + j) = *(A + i * n + j);
            *(R + i * n + j) = 1e-20;
        }
    for (K = 0; K < n; K++) {
        for (i = 0; i < K; i++)
            *(R + i * n + K) = innerMatColumnMAT(
                Q, n, i, K);  //	R(1:k-1,k) = Q(:,1:k-1)'*Q(:,k);
        for (i = 0; i < n; i++) {
            float QinR = 0;
            for (j = 0; j < K; j++)
                QinR = QinR + *(Q + i * n + j) * *(R + j * n + K);
            *(Q + i * n + K) = *(Q + i * n + K) - QinR;
        }
        *(R + K * n + K) = norm_Col(Q, n, n, K);
        for (i = 0; i < n; i++) {
            *(Q + i * n + K) = *(Q + i * n + K) / *(R + K * n + K);
        }
    }
    return (1);
}

void matSUB(float *mat1, float *mat2, float *result, int m, int n) {
    // Calculates mat1_m*n - mat2_m*n and reutrns the resultant matrix to result
    // Matrices should be the same size
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            result[i * n + j] = mat1[i * n + j] - mat2[i * n + j];
        }
    }
}

float norm_Col(float *A, int m, int n, int C) {
    float sum = 0;
    for (int i = 0; i < m; i++) {
        sum = sum + A[i * n + C] * A[i * n + C];
    }
    return sqrt(sum);
}

void backsubstitution(float *R, float *y_Qt, float *x_hat, int n) {
    for (int d = 0; d < n; d++) x_hat[d] = 0;
    for (int i = n - 1; i >= 0; i--) {
        x_hat[i] = y_Qt[i] / R[i * n + i];
        for (int j = 0; j < i; j++) {
            y_Qt[j] = y_Qt[j] - R[j * n + i] * x_hat[i];
        }
    }
}

float innerMatColumnMAT(float *mat, int n, int C1, int C2) {
    // n is size, C1 and C2 are columns that are correlated
    float sum = 0;
    for (int i = 0; i < n; i++) {
        sum += mat[i * n + C1] * mat[i * n + C2];
    }
    return sum;
}

int Union(int *vec, int newval, int *number_of_finded) {
    // Using this function values of index are sorted and there is no need to
    // check for repitition
    for (int i = 0; i < *number_of_finded; i++) {
        if (vec[i] == newval) return 0;  // If there is repeatition, ignore
    }

    // if (*(vec + number_of_finded - 1) != newval)
    {
        vec[(*number_of_finded)++] =
            newval;  // If it is bigger than all put it in the righmost place
        std::sort(vec, vec + *number_of_finded);
    }
    return (0);
}

float MSE(float *a, float *b, int Length) {
    int l;
    float temp;
    float mse = 0;
    for (l = 0; l < Length; l++) {
        temp = a[l] - b[l];
        mse += temp * temp;
    }
    return mse;
}

float SNR(float *a, float *b, int Length) {
    int l;
    int i;
    int temp;
    float mse = MSE(a, b, Length);
    float signal_power = 0;
    for (i = 0; i < Length; i++) signal_power += a[i] * a[i];

    printf("mse : %f \n", mse);
    printf("signal_power : %d \n", (int)signal_power);
    float snr = 10 * log10(signal_power / mse);
    printf("SNR : %f \n", snr);
    return snr;
}

void decompress(float *A, float *y, float *x_hat, int m, int n) {
    /* BEGINNIG OF OMP ALGORITHM */
    {
        float r[m], norm_of_column[n], correlation[n];
        int finded_index[n];

        int number_of_finded = 0;

        for (int i = 0; i < n; i++) {
            x_hat[i] = 0;
            finded_index[i] =
                99999999;  // Initialize with some value less than all
            norm_of_column[i] =
                norm_Col((float *)A, m, n, i);  // Calculate norm of column of C
        }

        for (int i = 0; i < m; i++) {  // in matlab code x meant to be y
            r[i] = y[i];               // r = x;
        }

        start();
        for (int iii = 0; iii < 5; iii++)
            // main loop,  q is number of expected coefficients or iterations
            for (int ii = 0; ii < 4; ii++) {
                for (int i = 0; i < n; i++) {  //// IMPORTANT LOOP
                    correlation[i] =
                        fabs((innerMulColumn((float *)A, (float *)r, m, n, i)) /
                             (norm_of_column[i]));
                }
                int max_value_index =
                    max_index((float *)correlation, n);  // find(f == max(f));
                Union(finded_index, max_value_index, &number_of_finded);

                {
                    // Solve_LS_with_finding_columns((float *)A, y, x_hat, (int
                    // *)finded_index, number_of_finded, m, n);
                    /* Start of Solve Least Sq. Algorithm */
                    float Acopy[m * number_of_finded];
                    float At[number_of_finded * m];
                    float A_square[number_of_finded * number_of_finded];
                    float Q[number_of_finded * number_of_finded];
                    float R[number_of_finded * number_of_finded];
                    float Qt[number_of_finded * number_of_finded];
                    float yQt[number_of_finded];
                    // Defining a two D array
                    /*float *Acopy = (float *)calloc(M * number_of_finded,
                    sizeof(float)); float *At = (float *)calloc(M *
                    number_of_finded, sizeof(float)); //Transpose of A copy
                    float *A_square = (float *)calloc(number_of_finded
                    *number_of_finded, sizeof(float)); //Transpose of A copy
                    float *Q = (float *)calloc(number_of_finded *
                    number_of_finded, sizeof(float)); //Transpose of A copy
                    float *R = (float *)calloc(number_of_finded
                    *number_of_finded, sizeof(float)); //Transpose of A copy
                    float *Qt = (float
                    *)calloc(number_of_finded*number_of_finded, sizeof(float));
                    //Transpose of Q copy float *yQt = (float
                    *)calloc(number_of_finded, sizeof(float)); //Transpose of A
                    copy
                    */
                    //                printf("%d\n", ii);
                    float y_p[number_of_finded];  // will be used as At * y
                                                  // (before Jacobi)
                    float x_hat_temp[m];

                    //                printf("%d\n", number_of_finded);
                    //                for(int i = 0; i < number_of_finded; ++i)
                    //                    printf("%d ", finded_index[i]);
                    //                printf("\n");

                    for (int i = 0; i < m;
                         i++)  // Get a copy of A with only selected index
                        for (int j = 0; j < number_of_finded; j++) {
                            Acopy[i * number_of_finded + j] =
                                A[i * n + finded_index[j]];
                        }

                    //                printf("%d\n", ii);
                    transpose((float *)Acopy, (float *)At, m, number_of_finded);

                    matMul((float *)At, y, (float *)y_p, number_of_finded, m,
                           1);  // At*y
                    matMul((float *)At, (float *)Acopy, (float *)A_square,
                           number_of_finded, m, number_of_finded);

                    QR((float *)A_square, (float *)Q, (float *)R,
                       number_of_finded);

                    transpose((float *)Q, (float *)Qt, number_of_finded,
                              number_of_finded);
                    /*for (int i = 0;i<number_of_finded;i++)
                            for (int j = 0; j < number_of_finded;j++)
                            {
                                    Qt[j][i] = Q[i][j];
                            }*/
                    matMul((float *)Qt, (float *)y_p, (float *)yQt,
                           number_of_finded, number_of_finded, 1);

                    backsubstitution((float *)R, (float *)yQt,
                                     (float *)x_hat_temp, number_of_finded);

                    // Jaocabi(A_square, y_p, x_hat_temp, number_of_finded,
                    // 100); // Solve the least squar for and get the results in
                    // x, of course length of x_hat_temp is more than necessary
                    // but we know how many of the are valid with
                    // number_of_finded

                    for (int i = 0; i < number_of_finded; i++)
                        x_hat[finded_index[i]] =
                            x_hat_temp[i];  // put the right value in the right
                                            // positions

                    //                printf("%d\n", ii);

                    float A_x_h[m];
                    matMul((float *)A, x_hat, A_x_h, m, n, 1);

                    matSUB(y, A_x_h, r, m, 1);  // r = y - A*s_hat;
                }
            } /*END OF OMP SECTION*/
    }
    stop();
}

};  // namespace omp
