#pragma once
#include "jely/math/Vec3.hpp"
#include <array>

namespace jely {
struct Rotation {
    double w=1,x=0,y=0,z=0;
    Vec3 apply(Vec3 v) const {const Vec3 q{x,y,z};return v+2.0*q.cross(q.cross(v)+v*w);}
};
// Horn's proper-rotation quaternion fit. Jacobi diagonalization handles even a 180-degree
// turn/reflection without a singular polar inverse or an identity-seeded iteration stall.
inline Rotation rigidRotation(const std::array<Vec3,3>& columns) {
    const double a00=columns[0].x,a10=columns[0].y,a20=columns[0].z;
    const double a01=columns[1].x,a11=columns[1].y,a21=columns[1].z;
    const double a02=columns[2].x,a12=columns[2].y,a22=columns[2].z;
    double matrix[4][4]{{a00+a11+a22,a21-a12,a02-a20,a10-a01},
        {a21-a12,a00-a11-a22,a01+a10,a02+a20},
        {a02-a20,a01+a10,-a00+a11-a22,a12+a21},
        {a10-a01,a02+a20,a12+a21,-a00-a11+a22}};
    double vectors[4][4]{};for(int i=0;i<4;i++)vectors[i][i]=1;
    for(int sweep=0;sweep<10;sweep++) {
        double maximum=0,scale=1e-30;for(int i=0;i<4;i++)scale=std::max(scale,std::abs(matrix[i][i]));
        for(int p=0;p<4;p++)for(int q=p+1;q<4;q++) {
            const double off=matrix[p][q];maximum=std::max(maximum,std::abs(off));
            if(std::abs(off)<scale*1e-14)continue;
            const double tau=(matrix[q][q]-matrix[p][p])/(2*off);
            const double t=std::copysign(1.0,tau)/(std::abs(tau)+std::hypot(1.0,tau));
            const double c=1/std::sqrt(1+t*t),s=t*c;
            matrix[p][p]-=t*off;matrix[q][q]+=t*off;matrix[p][q]=matrix[q][p]=0;
            for(int k=0;k<4;k++)if(k!=p&&k!=q){const double u=matrix[k][p],v=matrix[k][q];matrix[k][p]=matrix[p][k]=c*u-s*v;matrix[k][q]=matrix[q][k]=s*u+c*v;}
            for(int k=0;k<4;k++){const double u=vectors[k][p],v=vectors[k][q];vectors[k][p]=c*u-s*v;vectors[k][q]=s*u+c*v;}
        }
        if(maximum<scale*1e-13)break;
    }
    int largest=0;for(int i=1;i<4;i++)if(matrix[i][i]>matrix[largest][largest])largest=i;
    return {vectors[0][largest],vectors[1][largest],vectors[2][largest],vectors[3][largest]};
}
}
