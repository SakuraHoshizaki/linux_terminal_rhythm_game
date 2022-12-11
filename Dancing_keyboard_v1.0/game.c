#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/time.h>
#define BOARD_HEIGHT 29
#define BOARD_WIDTH 53
#define gotoxy(x,y) printf("\033[%d;%dH", (y), (x))

void printxyc(int x, int y, char c){
    printf("\033[%d;%dH%c", y, x, c);

}
void printxys(int x, int y, char *c){
    printf("\033[%d;%dH%s", y, x, c);

}

int get_pos(char *letters, char c){
    for(int i = 0; i < strlen(letters); i++){
        if(letters[i] == c){
            return i;
        }
    }
    return -1;
}

typedef struct board {
    char field[BOARD_HEIGHT][BOARD_WIDTH];
    char letters[26];
    
} Board;


typedef struct drop_struct
{
    int lefttime; //3000
    char letter;
} Drop_struct;

typedef struct drop_struct_queue{//data center?

    Drop_struct *d_structs[50];
    pthread_t *d_threads[50];
    Board *b;

    int perfect;
    int good;
    int bad;
    int miss;


} Queue;

void print_score(Queue *q){
    int score = q->perfect * 50 + q->good * 30 + q->bad*10;

    printf("\033[%d;%dHperfect: %d\033[%d;%dHgood: %d\033[%d;%dHbad: %d  \033[%d;%dHmiss: %d\033[%d;%dHTotal Score: %d",
            8, 75, q->perfect,
            9, 75, q->good,
            10, 75, q->bad,
            11, 75, q->miss,
            13, 75, score);
}


int add_drop_queue(Queue *q, Drop_struct *d, pthread_t *th){
    int i = 0;
    while(q->d_structs[i] != NULL){
        i++;
        if(i>99){
            return -1; // no place
        }
    }
    q->d_structs[i] = d;
    q->d_threads[i] = th;
    return i;
} 
 

int search_min_lefttime(Queue *q, char minletter){
    int pos = -1;
    int findl[20][2];//find multiple same letter

    for(int i = 0; i < 50; i++){
        if(q->d_structs[i] != NULL){
            if(q->d_structs[i]->letter == minletter){
                pos++;
                findl[pos][0] = q->d_structs[i]->lefttime;
                findl[pos][1] = i; // position in structs[] 
            }
        }
    }


    if(pos == -1){
        return -1;
    }
    else{
        int minpos = findl[0][1];
        int minleft = findl[0][0];
        for(int i = 0; i <= pos; i++){
            if(findl[i][0] < minleft){
                minleft = findl[i][0];
                minpos = findl[i][1];
            }
        }
        return minpos;
    }
}

int hantei(int i){
    if(i < 70 && i > -70){
        return 0;//perfect
    }
    else if(i < 150 && i > -150){
        return 1;//good
    }
    else if(i < 300 && i > -300){
        return 2;//bad
    }else{
        return 3;//not in line(no early miss)
    }

}

int get_drop_queue_time(Queue *q, int g_pos){

    if(q->d_structs[g_pos] == NULL)return 9000;
    pthread_cancel(*(q->d_threads[g_pos])); 
    
    int postime = q->d_structs[g_pos]->lefttime;
    

    q->d_structs[g_pos] = NULL;
    free(q->d_threads[g_pos]);
    q->d_threads[g_pos] = NULL;
    return postime;

}



void print_board(Board b) {

    for(int i = 0; i < BOARD_HEIGHT; i++) {        
        for(int j = 0; j < BOARD_WIDTH; j++){
            putchar(b.field[i][j]);
        }
        putchar('\r');
        putchar('\n');
    }    
}

void normal_board_thread(Board *b){
    while(1){  
        for(int i = 1; i < 52; i+=2){              
            printxyc(i+1, 26, '-');               
        }
        sleep(1);
       
    }
}

void get_keyboard_thread(Queue *q){
    char letters[26] = "qazwsxedcrfvtgbyhnujmikolp";
    while(1){
        char c = getchar();
        if(c == '`'){
            system("pulseaudio --kill");
            system("clear");
            exit(0);
        }
        int pos = get_pos(letters, c);
         if(pos != -1){
             
             int p = search_min_lefttime(q, c);
             if(p != -1){
                if(q->d_structs[p]->lefttime < 300){
                    int t = get_drop_queue_time(q, p);
                    int res = hantei(t);
                    if(res == 0){
                        printxys(60, 11, "perfect!  "); 
                        q->perfect ++;
                        print_score(q);
                    }else if(res ==1){
                        printxys(60, 11, "good!     ");
                        q->good ++; 
                        print_score(q);
                    }else if(res == 2){
                        printxys(60, 11, "bad!      ");
                        q->bad++;  
                        print_score(q);
                    }else{
                        printxys(60, 11, "miss!     ");
                        q->miss++;
                        print_score(q);
                    }
   
    		    for(int i = 21; i < 26; i++){
        		printxyc((pos+1)*2,i,' ');
    		    } 
    		    printxyc((pos+1)*2,26,'*');
                }
             }
         }
    }
}

//collect unclicked miss(also printing score)
void miss_thread(Queue *q){
    while(1){
        for(int i = 0; i < 50; i++){
            if(q->d_structs[i] != NULL){
                if(q->d_structs[i]->lefttime <= -300){

                    get_drop_queue_time(q,i);//kill
                    printxys(60, 11, "miss!   ");
                    q->miss ++;
                    print_score(q);
                }
            }
        }
        usleep(100000);
    }
}


void drop_letter_thread(Drop_struct *d){
    //pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, PTHREAD_CANCEL_DEFERRED);

    char letter = d->letter;
    char letters[26] = "qazwsxedcrfvtgbyhnujmikolp";
    int pos_wid = (get_pos(letters, letter)+1)*2;

    struct timeval t;
    gettimeofday(&t,NULL);
    long int strt = (long int)t.tv_sec*1000 + (long int)t.tv_usec/1000; //milisecond

    long int strleft = d->lefttime;
    int pos_hei = 0;
    int curt = strleft;

    while(curt > 0){
        gettimeofday(&t,NULL);
        curt = strleft - ((long int)t.tv_sec*1000 + (long int)t.tv_usec/1000 - strt);
        d->lefttime = curt; // call back

        pos_hei = 26 - curt/120;

        printxyc(pos_wid, pos_hei-1, ' ');
        printxyc(pos_wid, pos_hei, letter);
        usleep(1000);
        

    }

    while(curt > -300){ // wait 300msc when no action
        gettimeofday(&t,NULL);
        curt = strleft -  ((long int)t.tv_sec*1000 + (long int)t.tv_usec/1000 - strt);
        d->lefttime = curt; // call back
	usleep(1000);

    }

    printxyc(pos_wid, 25, ' ');// letter disappear
    printxyc(pos_wid, 26, '-');// letter disappear

    // miss
    gettimeofday(&t,NULL);
    curt = strleft -  ((long int)t.tv_sec*1000 + (long int)t.tv_usec/1000 - strt);
    d->lefttime = curt; //call back miss

    return;

}

void drop_letter_thread_editor(Drop_struct *d){
    //pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, PTHREAD_CANCEL_DEFERRED);

    char letter = d->letter;
    char letters[26] = "qazwsxedcrfvtgbyhnujmikolp";
    int pos_wid = (get_pos(letters, letter)+1)*2;

    struct timeval t;
    gettimeofday(&t,NULL);
    long int strt = (long int)t.tv_sec*1000 + (long int)t.tv_usec/1000; //milisecond

    long int strleft = d->lefttime;
    int pos_hei = 0;
    int curt = strleft;

    while(curt > 0){
        gettimeofday(&t,NULL);
        curt = strleft - ((long int)t.tv_sec*1000 + (long int)t.tv_usec/1000 - strt);
        d->lefttime = curt; // call back

        pos_hei = 26 - curt/120;

        printxyc(pos_wid, pos_hei-1, ' ');
        printxyc(pos_wid, pos_hei, letter);
        usleep(1000);
        

    }

    printxyc(pos_wid, 25, ' ');
    printxyc(pos_wid, 26, '*');
    usleep(500000);
    printxyc(pos_wid, 26, '-');
    return;
}

typedef struct mg_struct{
    char music[100];
    char chart[100];
} Mg_struct;


void main_game_thread(Mg_struct *mgs){  
    system("clear"); // hide cursor
    system("/bin/stty raw onlcr"); // ignor enter key
    system("/bin/stty -echo");// hide input
    system("setterm -cursor off"); // hide cursor  

    pthread_t get_keyboard, normal_board,mt;

    Board bb = {.field = {
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |", 
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|",  
        "|Q| | |W| | |E| | |R| | |T| | |Y| | |U| | |I| |O| |P|",
        "| |A| | |S| | |D| | |F| | |G| | |H| | |J| | |K| |L| |",
        "| | |Z| | |X| | |C| | |V| | |B| | |N| | |M| | | | | |",

    }
    ,.letters = "qazwsxedcrfvtgbyhnujmikolp"};
    print_board(bb);

    //read fumen file

    Queue dq = {.b = &bb,
    .perfect = 0,
    .good = 0,
    .bad = 0,
    .miss = 0
    };
    for(int i = 0; i < 50; i++){
        dq.d_structs[i] = NULL;        
        dq.d_threads[i] = NULL;        
    }
    FILE *music_file;
    music_file = fopen(mgs->chart,"r");
    char music_command[50];
    sprintf(music_command, "paplay %s &> /dev/null &",mgs->music);

    int total_notes;
    
    fscanf(music_file, "%d\n", &total_notes);
    
    pthread_create(&get_keyboard, NULL, (void*)get_keyboard_thread, (void*)&dq);
    pthread_create(&normal_board, NULL, (void*)normal_board_thread, (void*)&bb);
    pthread_create(&mt, NULL, (void*)miss_thread, (void*)&dq);
        
    //printxys(60,26,"success3!");

    Drop_struct *d1;
    pthread_t *dt1;
    int note_length;
    char ch;
    
    struct timeval t;
    //play music here;
    system(music_command);
    gettimeofday(&t,NULL);
    long int strt = (long int)t.tv_sec*1000 + (long int)t.tv_usec/1000;
    long int curt = (long int)t.tv_sec*1000 + (long int)t.tv_usec/1000 - strt;
    
    
     for(int i = 0; i < total_notes; i++){

         gettimeofday(&t,NULL);
         curt = (long int)t.tv_sec*1000 + (long int)t.tv_usec/1000 - strt;

         dt1 = (pthread_t*)malloc(sizeof(pthread_t));
         d1 = (Drop_struct*)malloc(sizeof(Drop_struct));
         
         fscanf(music_file, "%c%d\n", &ch,&note_length);
         while((note_length - curt) > 4500){
            sleep(1);
            gettimeofday(&t,NULL);
            curt = (long int)t.tv_sec*1000 + (long int)t.tv_usec/1000 - strt;
         }//slow monitoring
        
        while((note_length - curt) > 3100){
            usleep(100000);
            gettimeofday(&t,NULL);
            curt = (long int)t.tv_sec*1000 + (long int)t.tv_usec/1000 - strt;
         }//fast monitoring
        
         d1->lefttime = note_length - curt;
         d1->letter = ch;

         add_drop_queue(&dq, d1, dt1);
        
         pthread_create(dt1, NULL, (void*)drop_letter_thread, (void*)d1);

     }
    
    int endlen;
    fscanf(music_file, "%d\n", &endlen);
    while(1){
        gettimeofday(&t,NULL);
        curt = (long int)t.tv_sec*1000 + (long int)t.tv_usec/1000 - strt;
        if(curt > endlen){
            break;
        }
        sleep(3);
    }

    
    pthread_cancel(get_keyboard);
    pthread_cancel(normal_board);
    pthread_cancel(mt);
    printxys(5,10,"Game over! Press any key to return to title!");
    system("pulseaudio --kill");
    getchar();
    system("pulseaudio --kill");
    system("/bin/stty cooked");  // 後始末
    system("/bin/stty echo");
    system("setterm -cursor on");    

    return;
}

typedef struct creater_struct{
    char letters[3000];
    int timings[3000];
    int start_time;
    int start_pos;
    int is_alive;
}C_struct;

void fumen_creater_get_key_thread(C_struct *cs){//game avalible as adding new notes
    struct timeval t;
    int curt;
    int strt = cs->start_time;
    int curp = cs->start_pos;
    printxys(60,19,"Press '`' to stop editing.");
    while(curp < 3000){
        char ch = getchar();
        if(ch == '`'){
            system("pulseaudio --kill");
            cs->start_pos = curp;
            cs->is_alive = 0;
            return;
        }

        gettimeofday(&t,NULL);
        curt = (long int)t.tv_sec*1000 + t.tv_usec/1000 - strt;
        cs->letters[curp] = ch;
        cs->timings[curp] = curt;
        curp++;
    }
    printxys(60,20,"Reached maximum note count(3000)!!");
    getchar();
    cs->start_pos = curp;
    return;
}




void fumen_creater_thread(Mg_struct *mgs){
    system("clear");
    FILE *chart_file;
    char music_command[50];
    sprintf(music_command, "paplay %s &> /dev/null &", mgs->music);

    Board bb = {.field = {
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |", 
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|",  
        "|Q| | |W| | |E| | |R| | |T| | |Y| | |U| | |I| |O| |P|",
        "| |A| | |S| | |D| | |F| | |G| | |H| | |J| | |K| |L| |",
        "| | |Z| | |X| | |C| | |V| | |B| | |N| | |M| | | | | |",

    }
    ,.letters = "qazwsxedcrfvtgbyhnujmikolp"};

    chart_file = fopen(mgs->chart,"r");
        int note_count;
        fscanf(chart_file, "%d\n", &note_count);
        int new_note_count = note_count;
        char note_letters[3000];
        int note_timings[3000];

        for(int i = 0; i < note_count; i++){
            fscanf(chart_file, "%c%d\n", &note_letters[i], &note_timings[i]);
        }
        
        int endlen;
        fscanf(chart_file, "%d\n", &endlen);
        //end of scanning        
        system("/bin/stty raw onlcr"); // ignor enter key
    	system("/bin/stty -echo");// hide input
    	system("setterm -cursor off"); // hide cursor  
        print_board(bb);
        //start music playing



        struct timeval t;

        Drop_struct *d;
        pthread_t *dt;

        system(music_command);
        gettimeofday(&t,NULL);
        long int strt = (long int)t.tv_sec*1000 + (long int)t.tv_usec/1000;
        long int curt = (long int)t.tv_sec*1000 + (long int)t.tv_usec/1000 - strt;
        //start keyboard capture thread
        pthread_t gt;
        C_struct c = {
            .start_time = strt,
            .start_pos = note_count,
            .is_alive = 1
        };
        for(int i = 0; i < note_count; i++){
            c.letters[i] = note_letters[i];
            c.timings[i] = note_timings[i];
        }

        pthread_create(&gt,NULL,(void*)fumen_creater_get_key_thread,(void*)&c);


        for(int i = 0; i < note_count; i++){
            gettimeofday(&t,NULL);
            curt = (long int)t.tv_sec*1000 + (long int)t.tv_usec/1000 - strt;
            dt = (pthread_t*)malloc(sizeof(pthread_t));
            d = (Drop_struct*)malloc(sizeof(Drop_struct));

            while((note_timings[i] - curt) > 4500){
                sleep(1);
                gettimeofday(&t,NULL);
                curt = (long int)t.tv_sec*1000 + (long int)t.tv_usec/1000 - strt;
            }//slow monitoring
        
            while((note_timings[i] - curt) > 3100){
                usleep(100000);
                gettimeofday(&t,NULL);
                curt = (long int)t.tv_sec*1000 + (long int)t.tv_usec/1000 - strt;
            }//fast monitoring
        
            d->lefttime = note_timings[i] - curt;
            d->letter = note_letters[i];

            if(c.is_alive == 0){break;}

            pthread_create(dt, NULL, (void*)drop_letter_thread_editor, (void*)d);
            
        }
        
    	while(c.is_alive == 1){
            if(c.is_alive == 0){break;}
            gettimeofday(&t,NULL);
            curt = (long int)t.tv_sec*1000 + (long int)t.tv_usec/1000 - strt;
            if(curt > endlen){
                printxys(60,18,"Music ended. Press '`' to save and exit.");
                break;
            }
        sleep(3);
        }

        pthread_join(gt,NULL);

        system("pulseaudio --kill");
        system("/bin/stty cooked");  // 後始末
        system("/bin/stty echo");
        system("setterm -cursor on"); 

        system("clear");

        printxys(1,1,"Saving your files...");
        //sort
        for(int i = 0; i < c.start_pos; i++){
            for(int j = i; j < c.start_pos-1; j++){
                if(c.timings[j] > c.timings[j+1]){
                    int tempi = c.timings[j];
                    char tempc = c.letters[j];
                    c.timings[j] = c.timings[j+1];
                    c.letters[j] = c.letters[j+1];
                    c.timings[j+1] = tempi;
                    c.letters[j+1] = tempc;
                }
            }
        }

        fclose(chart_file);
        chart_file = fopen(mgs->chart,"w");
        fprintf(chart_file, "%d\n", c.start_pos);

        for(int i = 0; i < c.start_pos; i++){
            fprintf(chart_file,"%c%d\n", c.letters[i], c.timings[i]);
            gotoxy(80,11);
            printf("%c%d\n", c.letters[i], c.timings[i]);
        }
        fprintf(chart_file, "%d\n", endlen);

        printf("All your keyboard actions has beed added to the chart.\n");

        fclose(chart_file);
        return;
           
    

}

void random_game_thread(int *freq){
  
    //system("clear"); 
    //printf("If you are ready, press any key!");
    //int iii = getchar();
    system("clear");
    system("/bin/stty raw onlcr"); // ignor enter key
    system("/bin/stty -echo");// hide input
    system("setterm -cursor off"); // hide cursor  

    pthread_t get_keyboard, normal_board,mt;

    Board bb = {.field = {
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |", 
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "| | | | | | | | | | | | | | | | | | | | | | | | | | |",
        "|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|",  
        "|Q| | |W| | |E| | |R| | |T| | |Y| | |U| | |I| |O| |P|",
        "| |A| | |S| | |D| | |F| | |G| | |H| | |J| | |K| |L| |",
        "| | |Z| | |X| | |C| | |V| | |B| | |N| | |M| | | | | |",

    }
    ,.letters = "qazwsxedcrfvtgbyhnujmikolp"};
    print_board(bb);

    //read fumen file

    Queue dq = {.b = &bb,
    .perfect = 0,
    .good = 0,
    .bad = 0,
    .miss = 0
    };
    for(int i = 0; i < 50; i++){
        dq.d_structs[i] = NULL;        
        dq.d_threads[i] = NULL;        
    }

    pthread_create(&get_keyboard, NULL, (void*)get_keyboard_thread, (void*)&dq);
    pthread_create(&normal_board, NULL, (void*)normal_board_thread, (void*)&bb);
    pthread_create(&mt, NULL, (void*)miss_thread, (void*)&dq);
        
    Drop_struct *d1;
    pthread_t *dt1;

     for(int i = 0; i < 100; i++){

         dt1 = (pthread_t*)malloc(sizeof(pthread_t));
         d1 = (Drop_struct*)malloc(sizeof(Drop_struct));

         char ch = (char)(((int)'a') + rand() % 25);
        
         d1->lefttime = 3000;
         d1->letter = ch;

         add_drop_queue(&dq, d1, dt1);
        
         pthread_create(dt1, NULL, (void*)drop_letter_thread, (void*)d1);
         usleep(*freq);
     }

    while(1){
        int num = dq.perfect + dq.good + dq.bad + dq.miss;
        if(num == 100){
            break;
        }
        sleep(3);
    }
    pthread_cancel(get_keyboard);
    pthread_cancel(normal_board);
    pthread_cancel(mt);
    printxys(10,10,"Game over! Press any key to return to title!");
    getchar();
    system("/bin/stty cooked");  // 後始末
    system("/bin/stty echo");
    system("setterm -cursor on");    

    return;
}



int main (int argc, char *argv[]) {



start:

    system("clear");
    printf("Welcome to Dancing Keyboard v1.0!\r\nPlease confirm the size of your terminal window is bigger than 100*40 or the game may crush\r\n\r\nInput a number to choose a mode!\r\n\r\n");
    printf("1: Start a new game  2: Edit your charts  3: 100 Random keyboard practice\r\n\r\nInput other keys to exit the program.");
    

    int istart;
    scanf("%d", &istart);

    if(istart == 1){


    char music_list[100][100];
    char chart_list[20][100];

    DIR *songdir;
    struct dirent *dp;
    songdir = opendir("./songs/");
    int song_count = 0;
    char k[10];

choosemusic_s:

    while(1){

        dp = readdir(songdir);
        if(dp == NULL)break;
        

        if(strlen(dp->d_name) > 20 || strlen(dp -> d_name) < 4){
            continue;
        }

        for(int i = 0; i<4; i++){
            k[3-i] = dp->d_name[strlen(dp -> d_name) -1 -i];
        }
        if(strcmp(k,".wav")==0){

        strcpy(music_list[song_count], dp->d_name);
        song_count++;

        }

    }
       

closedir(songdir);
        system("clear");
        
        printf("Rhythm game mode:\r\n\r\nChoose a song:\r\n");

        for(int i = 0; i < song_count; i++){
            printf("%d. %s\r\n",i+1 ,music_list[i]);
        }

        int i_music_choose;
        scanf("%d", &i_music_choose);

        if(i_music_choose < 1 || i_music_choose > song_count){
            goto choosemusic_s;
        }
        
        char music_name[31];

        for(int i = 0; i < strlen(music_list[i_music_choose - 1]); i++){
            if(music_list[i_music_choose - 1][i] == '.'){
                music_name[i] = '\0';
                break;
            }
            music_name[i] = music_list[i_music_choose - 1][i];
        }//get music name

        DIR *chart_folder_dir;
        struct dirent *dpc;
        chart_folder_dir = opendir("./charts/");
        char chart_filename[100];
        char chart_folder[100];
        
        while(1){
            dpc = readdir(chart_folder_dir);
            if(dpc == NULL){
                printf("No Chart File Found!");
                break;
            }
            if(strcmp(dpc->d_name, music_name) == 0){
                sprintf(&chart_folder, "./charts/%s/", dpc->d_name);
                break;
            }

        }
        closedir(chart_folder_dir);
        

        DIR *chart_file_dir;
        chart_file_dir = opendir(chart_folder);
        struct dirent *dpcf;
        int chart_count = 0;
        while(1){
            dpcf = readdir(chart_file_dir);
            if(dpcf == NULL || chart_count > 18)break;
            if(strlen(dpcf->d_name) > 20 || strlen(dpcf -> d_name) < 4){
                continue;
            }
            strcpy(chart_list[chart_count], dpcf->d_name);
            chart_count++;
        }
        printf("Select a difficulty:\r\n");
        for(int i = 0; i < chart_count; i++){
            printf("%d. %s",i+1 , chart_list[i]);
        }
        printf("\r\n");
        int selected_chart;
        scanf("%d", &selected_chart);
        selected_chart -= 1;
        
        pthread_t mgt;

        Mg_struct mgs;
        sprintf(&(mgs.chart), "./charts/%s/%s", music_name, chart_list[selected_chart]);
        sprintf(&(mgs.music), "./songs/%s", music_list[i_music_choose-1]);
       
        pthread_create(&mgt,NULL,(void*)main_game_thread,(void*)&mgs);
        pthread_join(mgt,NULL);
        goto start;
    }

    else if(istart == 2){


    char music_list[100][100];
    char chart_list[20][100];

    DIR *songdir;
    struct dirent *dp;
    songdir = opendir("./songs/");
    int song_count = 0;
    char k[10];
choosemusic_c:

    while(1){

        dp = readdir(songdir);
        if(dp == NULL)break;
        

        if(strlen(dp->d_name) > 20 || strlen(dp -> d_name) < 4){
            continue;
        }

        for(int i = 0; i<4; i++){
            k[3-i] = dp->d_name[strlen(dp -> d_name) -1 -i];
        }
        if(strcmp(k,".wav")==0){

        strcpy(music_list[song_count], dp->d_name);
        song_count++;

        }

    }
       

closedir(songdir);
        system("clear");
        printf("Welcome to chart editor mode!\r\n\r\nIntroduction:\r\nThis mode will create a new chart if there is no chart file exists, or add something in an exist chart file.\r\nAfter choosing music, an automatically played game of the previous chart will start, while your actions will\r\nstarted to be recorded. ALL OF YOUR KEYBOARD ACTIONS will be automatically added to the previous chart, so \r\nplease DO NOT try to hit any existing notes dropping from the top, just hit the key you want to add to it at\r\nthe right time. In this demo to delete a note, you need to open the source name.txt file and delete the line\r\nwhich is not easy. So please be careful with it!\r\n\r\nNotes: the maximum of notes in one chart is 3000.\r\n\r\nChoose a song to start editing:\r\n");
                for(int i = 0; i < song_count; i++){
            printf("%d. %s\r\n",i+1 ,music_list[i]);
        }

        int i_music_choose;
        scanf("%d", &i_music_choose);

        if(i_music_choose < 1 || i_music_choose > song_count){
            goto choosemusic_s;
        }
        
        char music_name[50];

        for(int i = 0; i < strlen(music_list[i_music_choose - 1]); i++){
            if(music_list[i_music_choose - 1][i] == '.')break;
            music_name[i] = music_list[i_music_choose - 1][i];
        }//get music name

        DIR *chart_folder_dir;
        struct dirent *dpc;
        chart_folder_dir = opendir("./charts/");
        char chart_filename[50];
        char chart_folder[50];

        while(1){
            dpc = readdir(chart_folder_dir);
            if(dpc == NULL){
                printf("No Chart File Found!");
                break;
            }
            if(strcmp(dpc->d_name, music_name) == 0){
                sprintf(&chart_folder, "./charts/%s/", dpc->d_name);
                break;
            }

        }
        closedir(chart_folder_dir);

        DIR *chart_file_dir;
        chart_file_dir = opendir(chart_folder);
        struct dirent *dpcf;
        int chart_count = 0;
        while(1){
            dpcf = readdir(chart_file_dir);
            if(dpcf == NULL || chart_count > 18)break;
            if(strlen(dpcf->d_name) > 20 || strlen(dpcf -> d_name) < 4){
                continue;
            }
            strcpy(chart_list[chart_count], dpcf->d_name);
            chart_count++;
        }
        printf("Select a difficulty:\r\n");
        for(int i = 0; i < chart_count; i++){
            printf("%d. %s",i+1 , chart_list[i]);
        }
        int selected_chart;
        scanf("%d", &selected_chart);
        selected_chart --;

        
        
        pthread_t mgt;

        Mg_struct mgs;

        strcpy(mgs.chart, chart_list[selected_chart]);
        strcpy(mgs.music, music_list[i_music_choose]);

        

        pthread_create(&mgt,NULL,(void*)fumen_creater_thread,(void*)&mgs);
        pthread_join(mgt,NULL);
	goto start;

    }
    else if(istart = 3){
random_game:
        system("clear");
        printf("Input number of keys per second: (0.2<n<5.0)\r\n");
        double i_rgame;
        scanf("%lf", &i_rgame);

        if(i_rgame <= 0.2 || i_rgame >= 5.0){
            printf("Invalid input!");
            sleep(1);
            goto random_game;
        }
        int fr = (int)(1/i_rgame*1000000);
        pthread_t r_game;
        pthread_create(&r_game,NULL,(void*)random_game_thread,(void*)&fr);
        pthread_join(r_game,NULL);
        
     
    }
    else{
        return 0;
    }
}
