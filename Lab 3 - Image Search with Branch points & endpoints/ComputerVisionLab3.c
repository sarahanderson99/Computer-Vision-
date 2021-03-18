/*
 *   Sarah Anderson
 *   ECE 4310: Computer Vision
 *   Lab 3: Letters
 *   Due: September 29, 2020
 *   The purpose is to implement thinning, branchpoint and endpoint detection to recognize letters in an image of text
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Function Declarations
unsigned char *read_in_image(int rows, int cols, FILE *input);
void save_image(unsigned char *image, char *file_name, int rows, int cols);
unsigned char *original_image_threshold(unsigned char *original_image, int original_image_rows, int original_image_cols);
void get_transitions(unsigned char *image, int image_rows, int image_cols, int row, int col, int *mark_pixel, int *end_points, int *branch_points);
unsigned char *thinning(unsigned char *image, int image_rows, int image_cols);
unsigned char *get_end_and_branch_points(unsigned char *image, int image_rows, int image_cols);
void roc(unsigned char *msf_image, unsigned char *end_and_branch_point_image, int msf_rows, int msf_cols, int end_rows, int end_cols, char *file_name);
   
//Main
int main(int argc, char *argv[]){
    FILE *image_file, *msf_file;
    int IMAGE_ROWS, IMAGE_COLS, IMAGE_BYTES, MSF_ROWS, MSF_COLS, MSF_BYTES;
    char file_header[256];
    unsigned char *input;
    unsigned char *msf;
    unsigned char *orig_image_with_threshold;
    unsigned char *thinned;
    unsigned char *end_and_branch_points;

    if (argc != 4){
        printf("Usage: ./executable image_file msf_file ground_truth_file\n");
        exit(1);
    }

    image_file = fopen(argv[1], "rb");
    if (image_file == NULL){
        printf("Error, could not read input image file\n");
        exit(1);
    }
    
    fscanf(image_file, "%s %d %d %d\n", file_header, &IMAGE_COLS, &IMAGE_ROWS, &IMAGE_BYTES);
    
    if ((strcmp(file_header, "P5") != 0) || (IMAGE_BYTES != 255)){
        printf("Error, not a greyscale 8-bit PPM image\n");
        fclose(image_file);
        exit(1);
    }

    msf_file = fopen(argv[2], "rb");
    if (msf_file == NULL){
        printf("Error, could not read MSF image\n");
        exit(1);
    }
    
    fscanf(msf_file, "%s %d %d %d\n", file_header, &MSF_COLS, &MSF_ROWS, &MSF_BYTES);
    
    if ((strcmp(file_header, "P5") != 0) || (MSF_BYTES != 255)){
        printf("Error, not a greyscale 8-bit PPM image\n");
        fclose(image_file);
        fclose(msf_file);
        exit(1);
    }

    // reads in image and allocates memory for the images
    input = read_in_image(IMAGE_ROWS, IMAGE_COLS, image_file);
    msf = read_in_image(MSF_ROWS, MSF_COLS, msf_file);

    //Thresholds the orginal image to 128
    orig_image_with_threshold = original_image_threshold(input, IMAGE_ROWS, IMAGE_COLS);

    //thins the image
    thinned = thinning(orig_image_with_threshold, IMAGE_ROWS, IMAGE_COLS);
    //finds and counts the branch points and end points
    end_and_branch_points = get_end_and_branch_points(thinned, IMAGE_ROWS, IMAGE_COLS);
    //ROC calcs
    roc(msf, end_and_branch_points, MSF_ROWS, MSF_COLS, IMAGE_ROWS, IMAGE_COLS, argv[3]);

    save_image(orig_image_with_threshold, "threshold.ppm", IMAGE_ROWS, IMAGE_COLS);
    save_image(thinned, "thinned.ppm", IMAGE_ROWS, IMAGE_COLS);
    save_image(end_and_branch_points, "end_and_branchpoints.ppm", IMAGE_ROWS, IMAGE_COLS);
    
    return 0;
}

//reads in image  and creates memory for it
unsigned char *read_in_image(int rows, int cols, FILE *input){
    unsigned char *image;
    image = (unsigned char *)calloc(rows * cols, sizeof(unsigned char));
    fread(image, sizeof(unsigned char), rows * cols, input);
    fclose(input);
    
    return image;
}

//saves an image as a ppm
void save_image(unsigned char *image, char *file_name, int rows, int cols){
    FILE * file;
    
    file = fopen(file_name, "w");
    fprintf(file, "P5 %d %d 255\n", cols, rows);
    fwrite(image, rows * cols, sizeof(unsigned char), file);
    
    fclose(file);
}

//finds the orginal image at threshold 128
unsigned char *original_image_threshold(unsigned char *original_image, int original_image_rows, int original_image_cols){
    int i = 0;
    int threshold = 128;
    unsigned char *output_threshold_image;

    output_threshold_image = (unsigned char *)calloc(original_image_rows * original_image_cols, sizeof(unsigned char));

    for (i = 0; i < (original_image_rows * original_image_cols); i++){
        if (original_image[i] <= threshold){
            output_threshold_image[i] = 255;
        }
        else{
            output_threshold_image[i] = 0;
        }
    }
    
    return output_threshold_image;
}

//Determines if the pixel needs to be kept/deleted from thinning
void get_transitions(unsigned char *image, int image_rows, int image_cols, int row, int col, int *mark_pixel, int *end_points, int *branch_points){
    int A = 0, B = 0, C = 0, D = 0, i;
    int index = 0, index2 = 0, edge_to_nonedge = 0, edge_neighbors = 0, current_pixel = 0, next_pixel = 0;

    // top row
    for (i = (col - 1); i <= col; i++){
        index = ((row - 1) * image_cols) + i;
        current_pixel = image[index];
        index2 = ((row - 1) * image_cols) + (i + 1);
        next_pixel = image[index2];

        if ((current_pixel == 255) && (next_pixel == 0)){
            edge_to_nonedge++;
        }
        if (current_pixel == 255){
            edge_neighbors++;
        }
    }

    // right down row
    for (i = (row - 1); i <= row; i++){
        index = (i * image_cols) + (col + 1);
        current_pixel = image[index];
        index2 = ((i + 1) * image_cols) + (col + 1);
        next_pixel = image[index2];

        if ((current_pixel == 255) && (next_pixel == 0)){
            edge_to_nonedge++;
        }
        if (current_pixel == 255){
            edge_neighbors++;
        }
    }

    // bottom left row
    for (i = (col + 1); i > (col - 1); i--){
        index = ((row + 1) * image_cols) + i;
        current_pixel = image[index];
        index2 = ((row + 1) * image_cols) + (i - 1);
        next_pixel = image[index2];

        if ((current_pixel == 255) && (next_pixel == 0)){
            edge_to_nonedge++;
        }
        if (current_pixel == 255){
            edge_neighbors++;
        }
    }

    // left up row
    for (i = (row + 1); i > (row - 1); i--){
        index = (i * image_cols) + (col - 1);
        current_pixel = image[index];
        index2 = ((i - 1) * image_cols) + (col - 1);
        next_pixel = image[index2];

        if ((current_pixel == 255) && (next_pixel == 0)){
            edge_to_nonedge++;
        }
        if (current_pixel == 255){
            edge_neighbors++;
        }
    }

    A = image[((row - 1) * image_cols) + col];
    B = image[(row * image_cols) + (col + 1)];
    C = image[(row * image_cols) + (col - 1)];
    D = image[((row + 1) * image_cols) + col];

    if (edge_to_nonedge == 1){
        if ((edge_neighbors >= 2) && (edge_neighbors <= 6)){
            if ((A == 0) || (B == 0) || ((C == 0) && (D == 0))){
                *mark_pixel = 1;
            }
            else{
                *mark_pixel = 0;
            }
        }
        else{
            *mark_pixel = 0;
        }
    }
    else{
        *mark_pixel = 0;
    }

    if (edge_to_nonedge == 1){
        *end_points = 1;
    }
    if (edge_to_nonedge > 2){
        *branch_points = 2;
    }
}

//thinning
unsigned char *thinning(unsigned char *image, int image_rows, int image_cols){
    int i, row, col;
    int is_pixel_marked = 0, index = 0, run_again  = 1, end_points = 0, branch_points = 0;
    unsigned char *thinned_image;
    unsigned char *temp_image;
    int marked_pixels = 0, ran_times = 0;

    // Allocate memory for thined image
    thinned_image = (unsigned char *)calloc(image_rows * image_cols, sizeof(unsigned char));
    temp_image = (unsigned char *)calloc(image_rows * image_cols, sizeof(unsigned char));

    // Copies orginal image to thinning image
    for (i = 0; i < (image_rows * image_cols); i++){
        thinned_image[i] = image[i];
        temp_image[i] = image[i];
    }

    // Thinning Algorithm
    while(run_again == 1){
        run_again = is_pixel_marked = marked_pixels = 0;
        ran_times++;

        for (row = 1; row < (image_rows - 1); row++){
            for (col = 1; col < (image_cols - 1); col++){
                index = (row * image_cols) + col;
                if (thinned_image[index] == 255){
                    is_pixel_marked = 0;
                    get_transitions(thinned_image, image_rows, image_cols, row,
                    col, &is_pixel_marked, &end_points, &branch_points);
                    if (is_pixel_marked == 1){
                        index = (row * image_cols) + col;
                        temp_image[index] = 0;
                        run_again = 1;
                        marked_pixels += 1;
                    }
                }
            }
        }

        printf("Pixels done on round ran %d: %d pixels\n", ran_times, marked_pixels);

        for (i = 0; i < (image_rows * image_cols); i++){
            thinned_image[i] = temp_image[i];
        }
    }
    return thinned_image;
}

//finds the endpoints and branch points
unsigned char *get_end_and_branch_points(unsigned char *image, int image_rows, int image_cols){
    int row = 0, i = 0, col = 0, index = 0, mark_pixel = 0;
    int end_points = 0, branch_points = 0;
    int num_of_endp = 0, num_of_branchp = 0;
    unsigned char *end_and_branch_point_image;
    unsigned char *thinned_with_points;

    end_and_branch_point_image = (unsigned char *)calloc(image_rows * image_cols, sizeof(unsigned char));
    thinned_with_points = (unsigned char *)calloc(image_rows * image_cols, sizeof(unsigned char));

    for (i = 0; i < (image_rows * image_cols); i++){
        thinned_with_points[i] = image[i];
    }

    for (row = 1; row < (image_rows - 1); row++){
        for (col = 1; col < (image_cols - 1); col++){
            index = (row * image_cols) + col;
            end_points = 0;
            branch_points = 0;

            if (image[index] == 255){
                get_transitions(image, image_rows, image_cols, row, col, &mark_pixel, &end_points, &branch_points);
                if (end_points == 1){
                    end_and_branch_point_image[index] = 50;
                    thinned_with_points[index] = 100;
                    num_of_endp += 1;
                }
                if (branch_points == 2){
                    end_and_branch_point_image[index] = 150;
                    thinned_with_points[index] = 200;
                    num_of_branchp += 1;
                }
           }
        }
    }

    printf("Number of found endpoints: %d | Number of found brachpoints: %d\n", num_of_endp, num_of_branchp);
    save_image(thinned_with_points, "thinned_points.ppm", image_rows, image_cols);

    return end_and_branch_point_image;
}

void roc(unsigned char *msf_image, unsigned char *end_and_branch_point_image, int msf_rows, int msf_cols, int end_rows, int end_cols, char *file_name){
    FILE *file, *csv_file;
    int i = 0, j = 0, rows = 0, cols = 0, row1 = 0, col1 = 0, tp = 0, fp = 0, fn = 0, tn = 0, threshold = 0, index = 0, found = 0;
    int end_points = 0, branch_points = 0;
    char current_character[2], desired_character[2];
    unsigned char *temp_image;
    
    strcpy(desired_character, "e");

    //reads in the ground truth file
    file = fopen(file_name, "r");
    if (file == NULL){
        printf("Error, could not read ground truth file\n");
        exit(1);
    }

    temp_image = (unsigned char *)calloc(msf_rows * msf_cols, sizeof(unsigned char));

    csv_file = fopen("roc.csv", "w");
    fprintf(csv_file, "Threshold,TP,FP,FN,TN,TPR,FPR,PPV\n"); //headers for CSV

    //finds the threshold for the image
    for (i = 0; i <= 250; i += 5){
        threshold = i;

        for (j = 0; j < (msf_rows * msf_cols); j++){
            if (msf_image[j] >= threshold){
                temp_image[j] = 255;
            }
            else{
                temp_image[j] = 0;
            }
        }
        
        //finds fp, tp, fn, fp from ground truth file
        while((fscanf(file, "%s %d %d\n", current_character, &cols, &rows)) != EOF){
            end_points = 0;
            branch_points = 0;
            found = 0;

            for (row1 = rows - 7; row1 <= (rows + 7); row1++){
                for (col1 = cols - 4; col1 <= (cols + 4); col1++){
                    index = (row1 * msf_cols) + col1;
                    if (temp_image[index] == 255){
                        found = 1;
                    }
                    if (end_and_branch_point_image[index] == 50){
                        end_points += 1;
                    }
                    if (end_and_branch_point_image[index] == 150){
                        branch_points += 1;
                    }
                }
            }

   
            if ((found == 1) && (end_points == 1) && (branch_points == 1)){
                found = 1;
            }
            else{
                found = 0;
            }

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
        }
        
        //prints the cnumbers to the csv file
        fprintf(csv_file, "%d,%d,%d,%d,%d,%.2f,%.2f,%.2f\n", threshold, tp, fp, fn, tn, tp/(double)(tp +fn ),fp/(double)(fp+tn), fp/(double)(tp+fp));
        tp = fp = fn = tn = 0;
        rewind(file);
    }

    fclose(file);
    fclose(csv_file);
}
