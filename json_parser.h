typedef enum { START, KEY, VALUE, STOP } parse_state;

#define MAX_DEPTH 10

char **KEYS;
int NB_KEYS;

int check_keyname(char *js, jsmntok_t *tokens, int token_index)
{
   int pos;
   const char delim[2] = "/";

   //puts("\n\n");

   //for (size_t i = 0; i < sizeof(KEYS)/sizeof(char *); i++)
   for (size_t i = 0; i < NB_KEYS; i++)
   {
      pos = token_index;

      //1) split the searched key in key/subkeys
      //   for example, if KEYS[i] = "id/ediResponse",
      //   we get search_keys[0] = "id" and search_keys[1] = "ediResponse"
      char *search_keys[MAX_DEPTH];
      char *search_key = NULL;
      int search_depth = 0;

      //need a copy of the searched key (KEYS[i]), as strtok modify the string at each iteration,and KEYS[i] can't be modified.
      char * key_copy = malloc(strlen(KEYS[i])+1);
      strcpy(key_copy, KEYS[i]);
      search_key = strtok(key_copy, delim);
      while( search_key != NULL)
      {
         search_keys[search_depth] = malloc(strlen(search_key) + 1);
         strcpy(search_keys[search_depth], search_key);
         search_depth++;
         search_key = strtok(NULL, delim);
      }
      free(key_copy);

      /*for (int k = 0; k<search_depth ; k++)
      {
         printf("search_key[%d]=%s\n",k,search_keys[k]);
         fflush(stdout);
      }*/

      //2) we do the same with the token (t->end - t->start gets the actual key), by accessing parent with t->parent until we find a string (name of the object) or we reach the begin (t->parent = -1)
      char *current_keys[MAX_DEPTH];
      int current_depth = 0;

      while (pos != -1)
      {
         if (tokens[pos].type == JSMN_STRING)
         {
             current_keys[current_depth] = json_token_tostr(js,&tokens[pos]);
             current_depth++;
         }
         pos = tokens[pos].parent;
      }

      /*for (int k = 0; k<current_depth ; k++)
      {
         printf("found_key[%d]=%s\n",k,current_keys[k]);
         fflush(stdout);
      }*/

      //3) just compare values
      //printf("search_depth=%d current_depth=%d\n",search_depth,current_depth);
      if (search_depth == current_depth)
      {
         int k = 0;
         while (k < search_depth)
         {
            if (strcmp(search_keys[k],current_keys[search_depth-k-1]) != 0)
            {
               break;
            }
            else
            {
               k++;
            }
         }
         if (k == search_depth)
         {
            //puts("!!!!!!!match!!!!!");
            return i;
         }
      }
   }

   return -1;
}

int print_token(jsmntok_t *t, char *js,int number,parse_state actual,parse_state next)
{
   char type[20];
   switch (t->type)
   {
      case 0 :
         strcpy(type,"JSMN_UNDEFINED");
         break;
      case 1 :
         strcpy(type,"JSMN_OBJECT");
         break;
      case 2 :
         strcpy(type,"JSMN_ARRAY");
         break;
      case 3 :
         strcpy(type,"JSMN_STRING");
         break;
      case 4 :
         strcpy(type,"JSMN_PRIMITIVE");
         break;
      default:
         log_die("Invalid type %u", t->type);
   }

   char actual_state[20];
   switch (actual)
   {
     case START :
       strcpy(actual_state,"START");
       break;
     case STOP :
       strcpy(actual_state,"STOP");
       break;
     case KEY :
       strcpy(actual_state,"KEY");
       break;
     case VALUE :
       strcpy(actual_state,"VALUE");
       break;
       break;
     default:
       log_die("Invalid type %u", actual);
   }

   char next_state[20];
   switch (next)
   {
     case START :
       strcpy(next_state,"START");
       break;
     case STOP :
       strcpy(next_state,"STOP");
       break;
     case KEY :
       strcpy(next_state,"KEY");
       break;
     case VALUE :
       strcpy(next_state,"VALUE");
       break;
     default:
       log_die("Invalid type %u", next_state);
   }

   char debug_content[t->end - t->start +1];
   memcpy(debug_content, &js[t->start],t->end - t->start);
   debug_content[t->end - t->start] = '\0';

   printf("#######TOKEN %d (actual=%s, next=%s, parent : %d) :\ntype : %s\nstart : %d\nend : %d\nsize : %d\nvalue :%s\n#######\n",number,actual_state,next_state,t->parent,type,t->start,t->end,t->size,debug_content);
   return 0;
}

char *full_path_key(jsmntok_t *t_value, jsmntok_t *tokens, char* js, char *fullpath){
   jsmntok_t *t_parent;

   if (t_value->parent == -1) {
      return fullpath;
   }

   t_parent = &tokens[t_value->parent];
   char *res;
   if (t_parent->type == JSMN_STRING) {

      char key[t_parent->end - t_parent->start +1];
      memcpy(key, &js[t_parent->start],t_parent->end - t_parent->start);
      key[t_parent->end - t_parent->start] = '\0';

      res = malloc(strlen(fullpath)+strlen(key)+1);
      res[0] = '\0';
      strcat(res,key);
      strcat(res,fullpath);
      return full_path_key(t_parent,tokens,js,res);
   }

   if (t_parent->type == JSMN_ARRAY) {
      char array_index_string[12];
      sprintf(array_index_string, "%d", t_parent->array_index);

      res = malloc(strlen(fullpath)+strlen(array_index_string)+2);
      res[0] = '\0';
      strcat(res,"_");
      strcat(res,array_index_string);
      strcat(res,fullpath);
      return full_path_key(t_parent,tokens,js,res);
   }

   res = malloc(strlen(fullpath)+2);
   res[0] = '\0';
   strcat(res,"/");
   strcat(res,fullpath);

   return full_path_key(t_parent,tokens,js,res);
}

int print_key_value(jsmntok_t *t_value, char *js, jsmntok_t *tokens) {

   char *value;

   if (t_value->type == JSMN_ARRAY) {
      value = malloc(13*sizeof(char));
      sprintf(value, "%d", t_value->size);
   } else {
      value = malloc((t_value->end - t_value->start +1+1)*sizeof(char));
      memcpy(value, &js[t_value->start],t_value->end - t_value->start);
      value[t_value->end - t_value->start] = '\0';
   }

   printf("%s=%s\n",full_path_key(t_value,tokens,js,""),value);

   return 0;
}

int print_if_parent_is_interchange(jsmntok_t *t_value, char *js, jsmntok_t *tokens) {

   char *value;
   char *key;
   //char *key_interchange;

   //for the key
   jsmntok_t *t_parent;

   //the interchange object
   jsmntok_t *t_grandfather/*,*t_great_grandfather*/;

   t_parent = &tokens[t_value->parent];
   t_grandfather = &tokens[t_parent->parent];
   //t_great_grandfather = &tokens[t_grandfather->parent];

   //key_interchange = malloc((t_great_grandfather->end - t_great_grandfather->start +1+1)*sizeof(char));
   //memcpy(key_interchange, &js[t_great_grandfather->start],t_great_grandfather->end - t_great_grandfather->start);
   //key_interchange[t_great_grandfather->end - t_great_grandfather->start] = '\0';

   //if (strcmp(key_interchange,"interchange") == 0) {
   if (t_grandfather->parent == 1) {
      key = malloc((t_parent->end - t_parent->start +1+1)*sizeof(char));
      memcpy(key, &js[t_parent->start],t_parent->end - t_parent->start);
      key[t_parent->end - t_parent->start] = '\0';

      value = malloc((t_value->end - t_value->start +1+1)*sizeof(char));
      memcpy(value, &js[t_value->start],t_value->end - t_value->start);
      value[t_value->end - t_value->start] = '\0';

      printf("%s=%s\n",key,value);
      free(value);
      free(key);
   }
   //free(key_interchange);
   return 0;
}
