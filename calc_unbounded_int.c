#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

#include "unbounded_int.h"

typedef struct variable {
    char* name;
    unbounded_int value;
    struct variable* next;
}variable;

typedef struct {
    variable* first;
}list;

static void help();
static int isInputOption(const char* str);
static int isOutputOption(const char* str);
void destroy_list(list* l);
void destroy_variable(variable* var);
variable* create_variable(char* name, unbounded_int val);
variable* get_variable(list* l, char* name);
variable* add_variable(list* l, char* name, char* val);
int affect_var(variable* v, char* str);
char* nextWord(int begin, char* line, char delimiter);
char* readLine(FILE* source);

/*
////////////////////////////////////////////////////////////////////
ftell(SEEK_END)
et fseek pour recuperer directement le nombre d'octet d'un fichier
et donc recuperer sa taille.
On peut alors allouer directement un buffer à la bonne taille.
On pourrait donc rempalcer la methode ou je lis au fur et a mesure
le fichier et ou je fais des realloc de temps en temps
////////////////////////////////////////////////////////////////////

*/

int main(int argc, char* argv[]) {
    FILE* source  = NULL,
          *output = NULL;

    char* arg;
    for(int i=1;i<argc;i+=2) {
        arg = argv[i];
        if(isInputOption(arg)) {
            if(argc == i+1) {
                puts("No file after -i option.");
                help();
                exit(2);
            }
            arg = argv[i+1];
            source = fopen(arg, "r+");
            if(source == NULL) {
                printf("Erreur lors de l'ouverture du fichier : %s\n", arg);
                help();
                exit(3);
            }
        }
        else if(isOutputOption(arg)) {
            if(argc == i+1) {
                puts("No file after -o option.");
                help();
                exit(4);
            }
            arg = argv[i+1];
            // On ecrase le fichier et son contenu si il existe.
            // Sinon, on en cree un nouveau (w).
            output = fopen(arg, "w");
            if(output == NULL) {
                printf("Erreur lors de la creation du fichier : %s\n", arg);
                help();
                exit(5);
            }
        }
        else {
            printf("Argument inconnu : %s\n", arg);
            help();
            exit(-1);
        }
    }

    if(source == NULL) {
        source = stdin;
        printf("(Press Ctrl+D to stop the program)\n");
    }
    if(output == NULL)
        output = stdout;

/*
    char str[60];
    char* word;
    while(fgets(str, 60, source) != NULL) {
        word = getWord(str, ' ');
        printf("%s", word);
        free(word);
    }
*/
    list l = {.first = NULL};
    char* line, *word = NULL, *lastWord = NULL;
    variable* var;
    while(!feof(source)) {
        line = readLine(source);
        if(line == NULL)
            continue;
        for(int index=0;(word = nextWord(index, line, ' ')) != NULL;) {
            // On se decalle pour recuperer le prochain mot.
            index += strlen(word);
            while(line[index] == ' ' && line[index] != '\0')
                index++;
            
            if(lastWord != NULL && strcmp(lastWord, "print") == 0) {
                var = get_variable(&l, word);
                if(var != NULL) {
                    printf("%s -> ", var->name);
                    print_unbounded_int(var->value);
                }
                else {
                    printf("ERROR: cannot print variable\n");
                    return EXIT_FAILURE;
                }
            }
            // should be "contains ="
            else if(lastWord != NULL && strcmp(word, "=") == 0) {
                var = get_variable(&l, lastWord);
                if(var == NULL)
                    var = add_variable(&l, lastWord, "0");
                affect_var(var, line+index);
                free(word); // lastWord sera free a la sortie du for.
                break;
            }
            else {

            }

            if(lastWord != NULL)
                free(lastWord);

            lastWord = word;
        }
        if(lastWord != NULL) {
            free(lastWord);
            lastWord = NULL;
        }
        //puts("");
        free(line);
    }
    
    destroy_list(&l);

    fclose(source);
    fclose(output);
    return EXIT_SUCCESS;
}

static int isInputOption(const char* str) {
    return strcmp(str, "-i") == 0 ? 1 : 0;
}

static int isOutputOption(const char* str) {
    return strcmp(str, "-o") == 0 ? 1 : 0;
}

static void help() {
    puts("Help :");
    puts("\t./calc_unbounded_int -i <source> -o <output>");
}

// str est la chaine apres le '='
int affect_var(variable* v, char* str) {
    if(str == NULL || *str == '\0' || v == NULL)
        return 1;
    int index = 0;
    while(str[index] == ' ' && str[index] != '\0')
        index++;
    char* val = nextWord(index, str, ' '); // ' ' ou '\0'
    if(val == NULL)
        return 1;
    unbounded_int u1 = v->value;
    unbounded_int u2 = string2unbounded_int(val);
    free(val);
    unbounded_int sum = unbounded_int_somme(u1, u2);
    destroy_unbounded_int(u1);
    destroy_unbounded_int(u2);
    v->value = sum;
    return 0;
}

variable* create_variable(char* name, unbounded_int val) {
    if(name == NULL || val.signe == '*')
        return NULL;
    variable* var = malloc(sizeof(variable));
    if(var == NULL)
        return NULL;
    char* n = malloc(sizeof(char)*(strlen(name)+1));
    if(n == NULL) {
        free(var);
        return NULL;
    }
    strcpy(n, name);
    var->name = n;
    var->value = val;
    var->next = NULL;
    return var;
}

void destroy_variable(variable* var) {
    if(var == NULL)
        return;
    free(var->name);
    destroy_unbounded_int(var->value);
    free(var);
}

void destroy_list(list* l) {
    variable* tmp = l->first;
    for(variable* var=l->first;var != NULL;var=tmp) {
        tmp = tmp->next;
        destroy_variable(var);
    }
}

int isEmpty(list* l) {
    return l->first == NULL;
}

variable* add_variable(list* l, char* name, char* val) {
    if(l == NULL || name == NULL || val == NULL)
        return NULL;
    unbounded_int u = string2unbounded_int(val);
    variable* var = create_variable(name, u);
    if(var == NULL) {
        destroy_unbounded_int(u);
        return NULL;
    }
    if(isEmpty(l)) {
        l->first = var;
        return var;
    }
    var->next = l->first;
    l->first = var;
    return var;
}

variable* get_variable(list* l, char* name) {
    if(name == NULL)
        return NULL;
    variable* tmp = l->first;
    for(;tmp != NULL && strcmp(tmp->name, name) != 0;tmp = tmp->next)
    ;
    return tmp;
}

char* nextWord(int begin, char* line, char delimiter) {
    if(line == NULL || begin < 0 || begin >= strlen(line))
        return NULL;
    char* pos = line + begin;
    char* end = strchr(pos, delimiter);
    if(end == NULL)
        end = strchr(pos, '\0');
    ptrdiff_t size = end - pos;
    char* str = malloc((size+1) * sizeof(char));
    memmove(str, pos, size);
    str[size] = '\0';
    
    return str;
}

char* readLine(FILE* source) {
    if(source == NULL)
        return NULL;
    int i=0, size = 30;
    char* line = malloc(size * sizeof(char));
    char c;
    for(;(c = fgetc(source)) != '\n' && !feof(source);i++) {
        if(i == size-1) { // On double la taille du buffer
            char* doubleLine = realloc(line, size * 2 * sizeof(char)); 
            if(doubleLine == NULL)
                return NULL;
            size *= 2;
            line = doubleLine;
        }
        line[i] = c;
    }

    line[i] = '\0';

    return line;
}