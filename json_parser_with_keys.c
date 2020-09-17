#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "jsmn.h"

#include "json.h"
#include "log.h"
#include "buf.h"

#include "json_parser.h"


/* to understand the different state ot the parser in the main switch, go to json.org and see the schematics that defines json langage
here is a resume :

object : n * key:value, seperated by , and delimited with {}. n can be 0

          ------------>---------------
{ --->----|----string---;---value----|---->---- }
             |---<------,------<---|

value :
--->------|  string  |--->------
          |  number  |
          |  object  |
          |  array   |
          |  true    |
          |  false   |
          |  null    |

A valid json file obeys these rules. It can start either by an objetc, or an array
To ease the parsing, we consider that values 'number', 'true', 'false', 'null' are primitives :
value :
---->-----|  string      |---->-----
          |  primitives  |
          |  object      |
          |  array       |

As a consequence, we can define 4 states for the parser :
- START entry point, will be array or object
- KEY beginning of an object
- VALUE a value as listed above -array, object or primitive
- STOP no more tokens, it's the end.
*/

//char URL[] = "http://example.org/test.json";

//char *KEYS[] = { "books/book/title", "books/book/author"};

int main(int argc, char *argv[])
{
   if (argc <= 2)
   {
      puts("usage :");
      puts("1) if build with -DURL_FLAG :");
      puts("./jsonparser URL KEY-1[KEY-2...KEY-N]");
      puts("2) Otherwise :");
      puts("./jsonparser /path/to/file KEY-1[KEY-2...KEY-N]");
      puts("You can also build the parser with the -DDEBUG_FLAG, to see full debug output of json token(s) retrieval");
      return 1;
   }

   //fetching searched key from argv to global tab KEYS.
   //Keys start at argv + 2, to get rid of argv[0] and argv[1] : prog name and url (or file path)
   KEYS = argv + 2;
   NB_KEYS = argc - 2;

#ifdef URL_FLAG
   //debug, used to fetch files from URL
   char *js = json_fetch_url(argv[1]);
#else
   //prod, file are fetched by other means and wrote to disk, the parser read them from disk
   char *js = json_fetch_file(argv[1]);
#endif

   jsmntok_t *tokens = json_tokenise(js);

   parse_state state = START;
   int print_value = 0;

   size_t object_tokens = 0;

   for (size_t i = 0, j = 1; j > 0; i++, j--)
   {
      jsmntok_t *t = &tokens[i];
      jsmntok_t *t_next = NULL;
      if ( j>0 )
      {
         t_next = &tokens[i+1];
      }

#ifdef DEBUG_FLAG
      parse_state actual_state = state;
#endif

      // Should never reach uninitialized tokens
      log_assert(t->start != -1 && t->end != -1);

      //if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
      //{
         j += t->size;
      //}

      switch (state)
      {
         case START:
            if (t->type != JSMN_OBJECT && t->type != JSMN_ARRAY)
            {
               log_die("Invalid response: root element must be an object or an array");
            }

            object_tokens = t->size;

            if (t->type == JSMN_OBJECT)
            {
               state = KEY;
            } else if (t->type == JSMN_ARRAY)
            {
               state = VALUE;
            }

            if (object_tokens == 0)
            {
               state = STOP;
            }

            //if (object_tokens % 2 != 0)
            //   log_die("Invalid response: object must have even number of children.");

            break;

         case KEY:
            //object_tokens--;

            if (t->type != JSMN_STRING)
            {
               log_die("Invalid response: object keys must be strings.");
            }

            state = VALUE;


            //here we look if we want to retrieve the value of the key
            int index = check_keyname(js,tokens,i);
            if (index >= 0)
            {
               printf("%s=",KEYS[index]);
               print_value = 1;
            }

            break;

         case VALUE:
            object_tokens--;

            if (print_value == 1)
            {
               char *str = json_token_tostr(js, t);
               puts(str);
               print_value = 0;
            }

            if (t->type == JSMN_OBJECT)
            {
               state = KEY;

               //null object{}
               if (t->size == 0)
               {
                  //if next token is not a key, we go to VALUE
                  if (!(t_next->type == JSMN_STRING && t_next->size == 1))
                  {
                     state = VALUE;
                  }
               }

               object_tokens += t->size;
            }
            else if (t->type == JSMN_ARRAY)
            {
               state = VALUE;

               //null array []
               if (t->size == 0)
               {
                  //if next token is a key, we go to KEY
                  if (t_next->type == JSMN_STRING && t_next->size == 1)
                  {
                     state = KEY;
                  }
               }
               object_tokens += t->size;
            }
            else if (t->type == JSMN_STRING || t->type == JSMN_PRIMITIVE)
            {
               //end of the three, we are reduced to primitive or string, and we have to know in which state we have to go next
               //if we have a key,then KEY, else value
               if (t_next->type == JSMN_STRING && t_next->size == 1)
               {
                  state = KEY;
               }
               else
               {
                  state = VALUE;
               }
            }

            if (object_tokens == 0)
            {
               state = STOP;
            }
            break;

         case STOP:
            // Just consume the tokens
            break;

         default:
            log_die("Invalid state %u", state);
      }
#ifdef DEBUG_FLAG
      print_token(t,js,i,actual_state,state);
#endif
   }

   return 0;
}
