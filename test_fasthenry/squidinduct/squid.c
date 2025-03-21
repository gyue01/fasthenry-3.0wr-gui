#include<stdlib.h>
#include<stdio.h>

#define sigma 58.0 //in um unit
#define lambda 0.158//penetration depth
float w=0.34;//width of the wire
float h=0.02;//thickness
float L=2.5;//out width of the loop
float jw=0.09;//width of the junction
float jh=0.16; //height of the junction

int nP=1;//number of points
FILE *fp;

void wp(float x, float y)
{
	fprintf(fp,"N%d x=%f y=%f\n",nP, x,y);
	nP++;
}


int main(int n, char **plist)
{
	float tx,ty;
	fp=fopen("squid_0.inp","w");
	fprintf(fp,"* created by YG squid pattern generator\n");
	//fprintf(fp,".unit um\n.default z=0 sigma=%f w=%f h=%f nhinc=2 nwinc=10\n",sigma,w,h);
	fprintf(fp,".unit um\n.default z=0 lambda=%f w=%f h=%f nhinc=2 nwinc=10\n",lambda,w,h);

	fprintf(fp,"\n");
	wp(0,0);
	ty=L+w/2;
	wp(0,ty);
	tx=L/2-w/2;
	wp(tx,ty);
	wp(L/2,ty);
	wp(tx,L+L/2-jh/2);
	wp(tx,L+L/2+jh/2);
	ty=2*L-w/2;
	wp(tx,ty);
	wp(tx,ty+w/2);
	wp(0,ty);
	wp(0,L*3);
	tx=-L/2+w/2;
	wp(tx,L+w/2);
	wp(-L/2,L+w/2);
	wp(tx,L+L/2-jh/2);
	wp(tx,L+L/2+jh/2);
	wp(tx,ty);
	wp(tx,ty+w/2);
	fprintf(fp,"E1 N1 N2\nE2 N2 N3\nE3 N3 N4\nE4 N3 N5\nE5 N5 N6 w=%f nwinc=%d\n",jw,60);
	fprintf(fp,"E6 N6 N7\nE7 N7 N8\nE8 N7 N9\nE9 N9 N10\nE10 N9 N15\nE11 N2 N11\nE12 N11 N12\nE13 N11 N13\n");
	fprintf(fp,"E14 N13 N14 w=%f nwinc=%d\nE15 N14 N15\nE16 N15 N16\n",jw,60);
	fprintf(fp,"\n.external N2 N9\n.freq fmin=1 fmax=1 ndec=1\n\n.end\n");

	printf("Hello\n");
	return 0;
}
