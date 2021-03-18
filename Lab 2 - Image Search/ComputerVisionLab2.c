/*
    Sarah Anderson
    Computer Vision: ECE 4310
    Lab 2: Optical Character Recognition
    Due: Sept. 15, 2020
*/

/*
 * This program will look for a specific image (which is given) in another image.
 * For example, this program will look for all of teh e's in a given text image
 * that includes e's. It also calculates the ROC with all of the other things (FP, TP, etc.)
 * Those calculations are then added to a CSV file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

//Function declarations
unsigned char *read_in(int rows, int cols, char file_header[], FILE *input_file);
int *convolution(unsigned char *input_image, int *zmt, int input_rows, int input_cols, int temp_rows, int temp_cols);
void find_min_and_max(int *min, int *max, int *image, int rows, int cols);
int *convolution(unsigned char *input_image, int *zmt, int input_rows, int input_cols, int temp_rows, int temp_cols);
unsigned char *normalize(int *image, int rows, int cols, int new_max, int new_min, int max, int min);
void roc(unsigned char *image, int image_rows, int image_cols, char *file_name);
int *zero_mean(unsigned char *temp_image, int temp_rows, int temp_cols);


int main(int argc, char *argv[]){
    FILE *image_file, *template_file, *output_file;
    int image_rows, image_cols, image_bytes, temp_rows, temp_cols, temp_bytes, min, max;
    char file_header[256];
    unsigned char *template_image;
    unsigned char *input_image;
    unsigned char *normalized_image;
    int *zero_mean_template, *convolution_image;
    
    //checking the amount of arguments of correct
    if(argc != 4){
        printf("Usage: ./executable image_file template_file ground_file\n");
        exit(1);
    }

    //opening up the input image and gives error code if not able to open
    image_file = fopen(argv[1], "rb");
    if (image_file == NULL){
        printf("Error, could not read image file\n");
        exit(1);
    }
    
    fscanf(image_file, "%s %d %d %d\n", file_header, &image_cols, &image_rows, &image_bytes);
    //checking to see if it is a grey scale image
    if ((strcmp(file_header, "P5") != 0) || (image_bytes != 255)){
        printf("Error, not a greyscale 8-bit PPM image\n");
         fclose(image_file);
        exit(1);
    }
    //opneing if found that is is a grey scale image
    template_file = fopen(argv[2], "rb");
    if (template_file == NULL){
        fclose(image_file);
        printf("Error, could not read template file\n");
        exit(1);
    }
    
    fscanf(template_file, "%s %d %d %d\n", file_header, &temp_cols, &temp_rows, &temp_bytes);
    //checking to see if it is a grey scale image
    if ((strcmp(file_header, "P5") != 0) || (temp_bytes != 255)){
        fclose(image_file);
        fclose(template_file);
        printf("Error, not a greyscale 8-bit PPM image\n");
        exit(1);
    }

    //reads in image and allocated memory for each of the images
    input_image = read_in(image_rows, image_cols, file_header, image_file);
    template_image = read_in(temp_rows, temp_cols, file_header, template_file);
    
    //finds the zero mean
    zero_mean_template = zero_mean(template_image, temp_rows, temp_cols);
    
    //finds the convolution of the input image and the zero mean
    convolution_image = convolution(input_image, zero_mean_template, image_rows, image_cols, temp_rows, temp_cols);

    //finds the min and max of the convolution image
    find_min_and_max(&min, &max, convolution_image, image_rows, image_cols);

    //finds the mormalized min and max of the image
    normalized_image = normalize(convolution_image, image_rows, image_cols, 255, 0, max, min);

    //output the new normalized image
    output_file = fopen("normalized.ppm", "w");
    fprintf(output_file, "P5 %d %d 255\n", image_cols, image_rows);
    fwrite(normalized_image, image_cols * image_rows, sizeof(unsigned char), output_file);
    fclose(output_file);
    
    //finds the ROC of the image (to help find the threshold)
    roc(normalized_image, image_rows, image_cols, argv[3]);

    return 0;
}

unsigned char *read_in(int rows, int cols, char file_header[], FILE *input_file){
    unsigned char *image;
    
    //allocates memory and reads in file that is inputed into the function
    image = (unsigned char *)calloc(rows * cols, sizeof(unsigned char));
    file_header[0] = fgetc(input_file);
    fread(image, sizeof(unsigned char), rows * cols, input_file);
    fclose(input_file);
    
    return image;
}

int *zero_mean(unsigned char *temp_image, int temp_rows, int temp_cols){
    int *zmt;
    int sum = 0, i = 0, mean = 0;
    
    //calculates the zero mean template used in the match filter
    zmt = (int *)calloc(temp_rows * temp_cols, sizeof(int));
    
    for (i = 0; i < (temp_rows * temp_cols); i++){
        sum += temp_image[i];
    }

    mean = sum / (temp_rows * temp_cols);

    for (i = 0; i < (temp_rows * temp_cols); i++){
        zmt[i] = temp_image[i] - mean;
    }
    return zmt;
}


int *convolution(unsigned char *input_image, int *zmt, int input_rows, int input_cols, int temp_rows, int temp_cols){
    int row1 = 0, row2 = 0, col1 = 0, col2 = 0, index = 0, index2 = 0, sum = 0;
    int *convolution_image;
    
    convolution_image = (int *)calloc(input_rows * input_cols, sizeof(int));
    
    //basic convolution (like the last lab)
    for (row1 = 7; row1 < (input_rows - 7); row1++){
        for (col1 = 4; col1 < (input_cols - 4); col1++){
            sum = 0;
            for(row2 = -7; row2 < (temp_rows - 7); row2++){
                for (col2 = -4; col2 < (temp_cols - 4); col2++){
                    index = (input_cols * (row1 + row2)) + (col1 + col2);
                    index2 = (temp_cols * (row2 + 7)) + (col2 + 4);
                    sum += input_image[index] * zmt[index2];
                }
            }
            index = (input_cols * row1) + col1;
            convolution_image[index] = sum;
        }
    }
    return convolution_image;
}

void find_min_and_max(int *min, int *max, int *image, int rows, int cols){
    int i;
    
    //finds the min and the max of the image
    *min = image[0];
    *max = image[0];
    for (i = 1; i < (rows * cols); i++){
        if (*min > image[i]){
            *min = image[i];
        }
        if (*max < image[i]){
            *max = image[i];
        }
    }
}

unsigned char *normalize(int *image, int rows, int cols, int new_max, int new_min, int max, int min){
    unsigned char *normalized_image;
    int i;
    
    normalized_image = (unsigned char *)calloc(rows * cols, sizeof(unsigned char));
    
    //255 is the new max and 0 is the new min
    for (i = 0; i < (rows * cols); i++){
        normalized_image[i] = ((image[i] - min)*(255 - 0)/(max-min)) + 0;
    }
    
    return normalized_image;
}


void roc(unsigned char *image, int image_rows, int image_cols, char *file_name){
    FILE *file, *csv_file;
    int i = 0, j = 0, rows = 0, cols = 0;
    int row1, col1;
    int tp = 0, fp = 0, fn = 0, tn = 0;
    int threshold = 0, index = 0, found = 0;
    char current_character[2], desired_character[2];
    unsigned char *temp_image;

    strcpy(desired_character, "e");

    // Read in ground truth file
    file = fopen(file_name, "r");
    
    if (file == NULL){
        printf("Error, could not read file\n");
        exit(1);
    }

    temp_image = (unsigned char *)calloc(image_rows * image_cols, sizeof(unsigned char));

    // creating the csv file and adding the headers to it
    csv_file = fopen("calculations.csv", "w");
    fprintf(csv_file, "Threshold,TP,FP,FN,TN,TPR,FPR,PPV\n");
    
    for (i = 0; i < 256; i += 5){
        threshold = i;
        for (j = 0; j < (image_rows * image_cols); j++){
            if (image[j] >= threshold){
                temp_image[j] = 255;
            }
            else{
                temp_image[j] = 0;
            }
        }

        //output a picture with the treshold found
        if (threshold == 200){
            csv_file=fopen("with_treshold.ppm","w");
            fprintf(csv_file,"P5 %d %d 255\n",image_cols,image_rows);
            fwrite(temp_image,image_cols*image_rows,1,csv_file);
            fclose(csv_file);
        }
         
        //marking where the "matches" are found
        while((fscanf(file, "%s %d %d\n", current_character, &cols, &rows)) != EOF){
            for (row1 = rows-7; row1 <= (rows + 7); row1++){
                for (col1 = cols-4; col1 <= (cols + 4); col1++){
                    index = (row1 * image_cols) + col1;
                    if (temp_image[index] == 255){
                        found = 1;
                    }
                }
            }
        
            // finding tp, fp, fn, and tn
            if ((found == 1) && (strcmp(current_character, desired_character) == 0)){
                tp++;
            }
            if ((found == 1) && (strcmp(current_character, desired_character) != 0)){
                fp++;
            }
            if ((found == 0) && (strcmp(current_character, desired_character) == 0)){
                fn++;
            }
            if ((found == 0) && (strcmp(current_character, desired_character) != 0)){
                tn++;
            }
            found = 0;
        }
        
        // writing values into csv file to be graphed later
        fprintf(csv_file, "%d,%d,%d,%d,%d,%.2f,%.2f,%.2f\n", threshold,
        tp, fp, fn, tn, tp/(double)(tp +fn ),fp/(double)(fp+tn), fp/(double)(tp+fp));
        tp = fp = fn = tn = 0;
        rewind(file);
    }
    
    fclose(file);
    fclose(csv_file);
}
