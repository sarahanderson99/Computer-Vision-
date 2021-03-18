/*
 *   Sarah Anderson
 *   ECE 4310: Computer Vision
 *   Lab 5: Active Contours
 *   Due: October 27, 2020
 *   The purpose of this code is to create a active countour around the image given (hawk.ppm)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SQR(x) ((x)*(x))
void ActiveContour(float *sobel_image, int *contour_row, int *contour_col, int ROWS, int COLS);
void InvertSobel(float *sobel_image, int ROWS, int COLS);
void Min(int *min_row, int *min_col, float *temp);
int Distance(int x1, int x2, int y1, int y2);
void InternalEnergy1(float *energy_array, int *contour_row, int *contour_col, int i);
void InternalEnergy2(float *energy_array, int *contour_row, int *contour_col, int avg, int i);
void ExternalEnergy(float *energy_array, float *sobel_image, int *contour_row, int *contour_col, int COLS, int i);
void SobelFilter(unsigned char *image, float *sobel_image, int ROWS, int COLS);
void Normalize(float *temp, int ROWS, int COLS, int range);
void MakeFinal(unsigned char *image, int *contour_row, int *contour_col, int ROWS, int COLS);
void Float2Unsigned(float *input, unsigned char *output, int ROWS, int COLS);

int main(){
	FILE *fpt;
	unsigned char *image, *sobel_char, *final;
	float *sobel_image;
	int	*contour_row, *contour_col;
	char header[320];
	int	ROWS,COLS,BYTES,i;

	if ((fpt=fopen("hawk.ppm","rb")) == NULL){
		printf("Unable to open hawk.ppm for reading\n");
		exit(0);
	}
    
	fscanf(fpt,"%s %d %d %d",header,&COLS,&ROWS,&BYTES);
    
	if (strcmp(header,"P5") != 0  ||  BYTES != 255){
		printf("Not a greyscale 8-bit PPM image\n");
		exit(0);
	}
    
	image=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
	header[0]=fgetc(fpt);	/* read white-space character that separates header */
	fread(image,1,COLS*ROWS,fpt);
	fclose(fpt);
	
	//Reads in countur points
	if ((fpt=fopen("intialcontour.txt", "rb")) == NULL){
		printf("Unable to open intial contours file\n");
		exit(0);
	}
	
	contour_row = (int *)calloc(42, sizeof(int));
	contour_col = (int *)calloc(42, sizeof(int));

	i = 0;
    //reads in contour columns and rows
	while(!feof(fpt)){
		fscanf(fpt, "%d %d\n", &contour_col[i], &contour_row[i]);
		i++;
	}
	
	final = (unsigned char *)calloc(ROWS*COLS, sizeof(unsigned char));
	for (i = 0; i < ROWS*COLS; i++)
		final[i] = image[i];
	
	MakeFinal(final, contour_row, contour_col, ROWS, COLS);
	fpt = fopen("init.ppm", "w");
	fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
	fwrite(final, COLS*ROWS, 1, fpt);
	fclose(fpt);
	
	sobel_image = (float *)calloc(ROWS*COLS, sizeof(float));
	
	SobelFilter(image, sobel_image, ROWS, COLS);
	Normalize(sobel_image, ROWS, COLS, 255);
	
	sobel_char = (unsigned char *)calloc(ROWS*COLS, sizeof(unsigned char));
	Float2Unsigned(sobel_image, sobel_char, ROWS, COLS);
	
	fpt = fopen("sobelout.ppm", "w");
	fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
	fwrite(sobel_char, COLS*ROWS, 1, fpt);
	fclose(fpt);
	
	ActiveContour(sobel_image, contour_row, contour_col, ROWS, COLS);
	
	final = (unsigned char *)calloc(ROWS*COLS, sizeof(unsigned char));
    for (i = 0; i < ROWS*COLS; i++){
		final[i] = image[i];
    }
	
	MakeFinal(final, contour_row, contour_col, ROWS, COLS);
	
	fpt = fopen("final.ppm", "w");
	fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
	fwrite(final, COLS*ROWS, 1, fpt);
	fclose(fpt);
	
	for (i = 0; i < 42; i++){
		printf("Contour Point %d: %d %d\n", i, contour_row[i], contour_col[i]);
	}
	return(0);
	
}

void MakeFinal(unsigned char *image, int *contour_row, int *contour_col, int ROWS, int COLS){
	int i, r, c;
	
	for (i = 0; i < 42; i++){
		for (r = -3; r <= 3; r++){
			image[(contour_row[i]+r)*COLS+contour_col[i]] = 255;
		}
		for (c = -3; c <= 3; c++){
			image[contour_row[i]*COLS+(contour_col[i]+c)] = 255;
		}
	}
	return;
}

void ActiveContour(float *sobel_image, int *contour_row, int *contour_col, int ROWS, int COLS){
	int i, j, k;
	
    float *internal1, *internal2, *external, *TotalEnergy;
	int   *temp_row, *temp_col;
	int   min_row, min_col;
	float average_distance;
	
	internal1 = (float *)calloc(7*7, sizeof(float));
	internal2 = (float *)calloc(7*7, sizeof(float));
	external = (float *)calloc(7*7, sizeof(float));
	TotalEnergy = (float *)calloc(7*7, sizeof(float));
	temp_row = (int *)calloc(42, sizeof(int));
	temp_col = (int *)calloc(42, sizeof(int));
	
	Normalize(sobel_image,  ROWS, COLS, 1);
	InvertSobel(sobel_image, ROWS, COLS);
	
	for (i = 0; i < 31; i++){
		average_distance = 0;
		for (j = 0; j < 41; j++){
			average_distance += Distance(contour_row[j+1], contour_row[j], contour_col[j+1], contour_col[j]);
		}
        
		average_distance += Distance(contour_row[0], contour_row[j], contour_col[0], contour_col[j]);
		average_distance = average_distance/42;
		
		for (j = 0; j < 42; j++){
			InternalEnergy1(internal1, contour_row, contour_col, j);
			InternalEnergy2(internal2, contour_row, contour_col, average_distance, j);
			ExternalEnergy(external, sobel_image, contour_row, contour_col, COLS, j);
			Normalize(internal1, 7, 7, 1);
			Normalize(internal2, 7, 7, 1);
			
			for (k = 0; k < 7*7; k++){
				TotalEnergy[k] = 2*internal1[k] + internal2[k] + external[k];
			}
			
			Min(&min_row, &min_col, TotalEnergy);
			
			temp_row[j] = contour_row[j]+min_row;
			temp_col[j] = contour_col[j]+min_col;
		}
		for (j = 0; j < 42; j++){
			contour_row[j] = temp_row[j];
			contour_col[j] = temp_col[j];
		}
	}
	
	return;
}

void InvertSobel(float *sobel_image, int ROWS, int COLS){
	int i;
	 for (i = 0; i < ROWS*COLS; i++){
		 sobel_image[i] = 1 - sobel_image[i];
	 }
	 
	 return;
}

void Min(int *min_row, int *min_col, float *temp){
	int i;
	float min = 20000.00;
	
	for (i = 0; i < 7*7; i++){
		if (temp[i] < min){
			min = temp[i];
			*min_row = (i/7) - 3;
			*min_col = (i%7) - 3;
		}
	}
	
}

int Distance(int x1, int x2, int y1, int y2){
	int ret_val;
	
	ret_val = sqrt(SQR(x2-x1)+SQR(y2-y1));
	
	return(ret_val);
}

void InternalEnergy1(float *energy_array, int *contour_row, int *contour_col, int i){
	int r, c;
	
	for (r = -3; r <= 3; r++){
		for (c = -3; c <= 3; c++){
			if (i != 41){
				energy_array[(r+3)*7+(c+3)] = Distance(contour_row[i+1], contour_row[i]+r, contour_col[i+1], contour_col[i]+c);
			}
			else{
				energy_array[(r+3)*7+(c+3)] = Distance(contour_row[0], contour_row[i]+r, contour_col[0], contour_col[i]+c);
			}
		}
	}
	return;
}

void InternalEnergy2(float *energy_array, int *contour_row, int *contour_col, int avg, int i){
	int r, c;
	
	for (r = -3; r <= 3; r++){
		for (c = -3; c <= 3; c++){
			if (i != 41){
				energy_array[(r+3)*7+(c+3)] = SQR(avg - Distance(contour_row[i+1], contour_row[i]+r, contour_col[i+1], contour_col[i]+c));
			}
			else{
				energy_array[(r+3)*7+(c+3)] = SQR(avg - Distance(contour_row[0], contour_row[i]+r, contour_col[0], contour_col[i]+c));
			}
		}
	}
	return;
}

void ExternalEnergy(float *energy_array, float *sobel_image, int *contour_row, int *contour_col, int COLS, int i){
	int r, c;
	for (r = -3; r <= 3; r++){
		for (c = -3; c <= 3; c++){
			energy_array[(r+3)*7+(c+3)] = SQR(sobel_image[(contour_row[i]+r)*COLS+(contour_col[i]+c)]);
		}
	}
	return;	
}

void SobelFilter(unsigned char *image, float *sobel_image, int ROWS, int COLS){
    const int F1[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
    const int F2[9] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};
	int   r, c, r2, c2;
	float sumF1, sumF2;
	
	for (r = 3; r < ROWS - 3; r++){
		for (c = 3; c < COLS - 3; c++){
			sumF1 = sumF2 = 0;
			for (r2 = -1; r2 <= 1; r2++){
				for (c2 = -1; c2 <= 1; c2++){
					sumF1 += image[(r+r2)*COLS+(c+c2)] * F1[(r2+1)*3+(c2+1)];
					sumF2 += image[(r+r2)*COLS+(c+c2)] * F2[(r2+1)*3+(c2+1)];
				}
			}
			
			sobel_image[r*COLS+c] = sqrt(SQR(sumF1) + SQR(sumF2));
		}
	}
	
	return;
}

void Normalize(float *temp, int ROWS, int COLS, int range){
  int i, max, min;
 
  max = 0;
  min  = 2000;
    
  for (i = 0; i < ROWS * COLS; i++){
    if (temp[i] < min){
      min = temp[i];
    }
    if (temp[i] > max){
      max = temp[i];
    }
  }
    //normalization
  for (i = 0; i < ROWS*COLS; i++){
    temp[i] = (temp[i] - min)*range/(max-min);
  }
 
  return;
}

void Float2Unsigned(float *input, unsigned char *output, int ROWS, int COLS){
	int i;
	for (i = 0; i < ROWS*COLS; i++){
		output[i] = input[i];
	}
	return;
}
	
