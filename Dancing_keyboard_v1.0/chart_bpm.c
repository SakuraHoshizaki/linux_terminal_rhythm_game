#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int main(int argc, char **argv){
    if(argc != 2){
        printf("Too much argument!");
        return 1;
    }
    if((strlen(argv[1])) > 49){
        printf("Filename too long!");
        return 1;
    }

    char in_filename[50];
    strcpy(in_filename, argv[1]);
    FILE *in_file;
    if((in_file = fopen(in_filename, "r")) == NULL){
        printf("No such file!");
        return 1;
    }

    char out_filename[50], out_music_name[21];
    int start_time,stop_timing;
    double bpm;
    fscanf(in_file, "%s\n", &out_filename);
    fscanf(in_file, "%s\n", &out_music_name);
    fscanf(in_file, "%lf\n", &bpm);
    fscanf(in_file, "%d", &start_time);
    
    double time_16 = 60000/bpm/4;
    char c;
    int note_count = 0;
    double cur_time = start_time;
    char out_notes[3000];
    double out_timings[3000];
    
    while((c = fgetc(in_file)) != EOF){
        if(((int)c) >= ((int)'a') && ((int)c) <= ((int)'z')){
            out_notes[note_count] = c;
            out_timings[note_count] = cur_time;
            cur_time += time_16;
            note_count ++;
        }else if(c == ' '){
            cur_time += time_16;
        }else if(c == '('){
            while((c = fgetc(in_file)) != ')'){//multiple click
                out_notes[note_count] = c;
                out_timings[note_count] = cur_time;
                note_count++;
            }
            cur_time += time_16;
        }
    }

    FILE *out_file;
    char out_path[40];
    char out_folder[40];
    sprintf(&out_folder, "./charts/%s", out_music_name);
    mkdir(out_folder,S_IRUSR | S_IWUSR | S_IXUSR |         /* rwx */
                S_IRGRP | S_IWGRP | S_IXGRP |         /* rwx */
                S_IROTH | S_IXOTH | S_IXOTH);
    sprintf(&out_path, "./charts/%s/%s", out_music_name, out_filename);
    out_file = fopen(out_path,"w");
    fprintf(out_file, "%d\n", note_count);
    for(int i = 0; i < note_count; i++){
        fprintf(out_file, "%c%d\n", out_notes[i], (int)out_timings[i]);
    }
    stop_timing = out_timings[note_count-1];
    fprintf(out_file, "%d\n", stop_timing);
    printf("Convert successed");
    return 0;   
}
