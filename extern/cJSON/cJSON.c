/*
  Copyright (c) 2009 Dave Gamble

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/* cJSON */
/* JSON parser in C. */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "cJSON.h"

static const char *ep;

const char *cJSON_GetErrorPtr(void) {return ep;}

static int cJSON_strcasecmp(const char *s1,const char *s2)
{
	if (!s1) return (s1==s2)?0:1;if (!s2) return 1;
	for(; tolower(*s1) == tolower(*s2); ++s1, ++s2)	if(*s1 == 0)	return 0;
	return tolower(*(const unsigned char *)s1) - tolower(*(const unsigned char *)s2);
}

static void *(*cJSON_malloc)(size_t sz) = malloc;
static void (*cJSON_free)(void *ptr) = free;

static char* cJSON_strdup(const char* str)
{
      size_t len;
      char* copy;

      len = strlen(str) + 1;
      if (!(copy = (char*)cJSON_malloc(len))) return 0;
      memcpy(copy,str,len);
      return copy;
}

void cJSON_InitHooks(cJSON_Hooks* hooks)
{
    if (!hooks) { /* Reset hooks */
        cJSON_malloc = malloc;
        cJSON_free = free;
        return;
    }

	cJSON_malloc = (hooks->malloc_fn)?hooks->malloc_fn:malloc;
	cJSON_free	 = (hooks->free_fn)?hooks->free_fn:free;
}

/* Internal constructor. */
static cJSON *cJSON_New_Item(void)
{
	cJSON* node = (cJSON*)cJSON_malloc(sizeof(cJSON));
	assert(node != NULL);
	memset(node,0,sizeof(cJSON));
	
	return node;
}

/* Delete a cJSON structure. */
void cJSON_Delete(cJSON *c)
{
	// nothing in an iterator type needs to be cleaned up
	if(c && c->type == cJSON_ArrayIterator)
	{
		cJSON_free(c);
	}
	else
	{
		cJSON *next;
		while (c)
		{
			next=c->next;
			if (!(c->type&cJSON_IsReference) && c->child) cJSON_Delete(c->child);
			if (!(c->type&cJSON_IsReference) && c->valuestring) cJSON_free(c->valuestring);
			if (c->string) cJSON_free(c->string);
			cJSON_free(c);
			c=next;
		}
	}
}

/* string stream for print methods */

typedef struct string_stream
{
	char* head;
	size_t count;
	size_t size;
} string_stream;

static int create_string_stream(string_stream* ss)
{
	assert(ss != NULL);
	assert(ss->head == NULL);
	assert(ss->count == 0);
	assert(ss->size == 0);

	ss->count = 0;
	ss->size = 16;
	ss->head = cJSON_malloc(ss->size);
	memset(ss->head, 0x00, ss->size);
	return ss->head != NULL;
}

static int grow_buffer(string_stream* ss)
{
	size_t new_size = 0;
	char* new_buffer = NULL;

	assert(ss != NULL);
	assert(ss->head != NULL);
	assert(ss->size == ss->count + 1);
	assert(ss->count != 0);

	new_size = ss->size * 2;
	new_buffer = cJSON_malloc(new_size);
	if(new_buffer == NULL)
	{
		return cJSON_False;
	}
	memcpy(new_buffer, ss->head, ss->size);
	// zeros at the end of it
	memset(new_buffer + ss->size, 0x00, ss->size);
	
	ss->size = new_size;
	cJSON_free(ss->head);
	ss->head = new_buffer;

	return cJSON_True;
}

#undef IfFailedReturn
#define IfFailedReturn(X)do{ if((X) == cJSON_False) { __debugbreak(); return cJSON_False;}} while(0,0)

// push a char
static int push_char(string_stream* ss, char c)
{
	assert(c != 0);
	assert(ss != NULL);

	if(ss->count == ss->size - 1)
	{
		IfFailedReturn(grow_buffer(ss));
	}

	ss->head[ss->count++] = c;

	return cJSON_True;
}

// push c string 
static int push_string(string_stream* ss, char* str)
{
	assert(ss != NULL);
	assert(str != NULL);

	while(*str != 0)
	{
		IfFailedReturn(push_char(ss, *str++));
	}
	return cJSON_True;
}

/* Parse the input text to generate a number, and populate the result into item. */
static const char *parse_number(cJSON *item,const char *num)
{
	double n=0,sign=1,scale=0;int subscale=0,signsubscale=1;

	/* Could use sscanf for this? */
	if (*num=='-') sign=-1,num++;	/* Has sign? */
	if (*num=='0') num++;			/* is zero */
	if (*num>='1' && *num<='9')	do	n=(n*10.0)+(*num++ -'0');	while (*num>='0' && *num<='9');	/* Number? */
	if (*num=='.' && num[1]>='0' && num[1]<='9') {num++;		do	n=(n*10.0)+(*num++ -'0'),scale--; while (*num>='0' && *num<='9');}	/* Fractional part? */
	if (*num=='e' || *num=='E')		/* Exponent? */
	{	num++;if (*num=='+') num++;	else if (*num=='-') signsubscale=-1,num++;		/* With sign? */
		while (*num>='0' && *num<='9') subscale=(subscale*10)+(*num++ - '0');	/* Number? */
	}

	n=sign*n*pow(10.0,(scale+subscale*signsubscale));	/* number = +/- number.fraction * 10^+/- exponent */
	
	item->valuedouble=n;
	item->valueint=(int)n;
	item->type=cJSON_Number;
	return num;
}

/* Render the number nicely from the given item into a string. */
static int print_number(string_stream* ss, cJSON *item)
{
	double d=item->valuedouble;
	char str[64] = {0}; /* This is a nice tradeoff. */

	if (fabs(((double)item->valueint)-d)<=DBL_EPSILON && d<=INT_MAX && d>=INT_MIN)
	{
		/* 2^64+1 can be represented in 21 chars. */
		sprintf(str,"%d",item->valueint);
	}
	else
	{
		if (fabs(floor(d)-d)<=DBL_EPSILON && fabs(d)<1.0e60)sprintf(str,"%.0f",d);
		else if (fabs(d)<1.0e-6 || fabs(d)>1.0e9)			sprintf(str,"%e",d);
		else												sprintf(str,"%f",d);
	}
	return push_string(ss, str);
}

/* Parse the input text into an unescaped cstring, and populate item. */
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
static const char *parse_string(cJSON *item,const char *str)
{
	const char *ptr=str+1;char *ptr2;char *out;int len=0;unsigned uc,uc2;
	if (*str!='\"') {ep=str;return 0;}	/* not a string! */
	
	while (*ptr!='\"' && *ptr && ++len) if (*ptr++ == '\\') ptr++;	/* Skip escaped quotes. */
	
	out=(char*)cJSON_malloc(len+1);	/* This is how long we need for the string, roughly. */
	if (!out) return 0;
	
	ptr=str+1;ptr2=out;
	while (*ptr!='\"' && *ptr)
	{
		if (*ptr!='\\') *ptr2++=*ptr++;
		else
		{
			ptr++;
			switch (*ptr)
			{
				case 'b': *ptr2++='\b';	break;
				case 'f': *ptr2++='\f';	break;
				case 'n': *ptr2++='\n';	break;
				case 'r': *ptr2++='\r';	break;
				case 't': *ptr2++='\t';	break;
				case 'u':	 /* transcode utf16 to utf8. */
					sscanf(ptr+1,"%4x",&uc);ptr+=4;	/* get the unicode char. */

					if ((uc>=0xDC00 && uc<=0xDFFF) || uc==0)	break;	/* check for invalid.	*/

					if (uc>=0xD800 && uc<=0xDBFF)	/* UTF16 surrogate pairs.	*/
					{
						if (ptr[1]!='\\' || ptr[2]!='u')	break;	/* missing second-half of surrogate.	*/
						sscanf(ptr+3,"%4x",&uc2);ptr+=6;
						if (uc2<0xDC00 || uc2>0xDFFF)		break;	/* invalid second-half of surrogate.	*/
						uc=0x10000 + (((uc&0x3FF)<<10) | (uc2&0x3FF));
					}

					len=4;if (uc<0x80) len=1;else if (uc<0x800) len=2;else if (uc<0x10000) len=3; ptr2+=len;
					
					switch (len) {
						case 4: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
						case 3: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
						case 2: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
						case 1: *--ptr2 =(uc | firstByteMark[len]);
					}
					ptr2+=len;
					break;
				default:  *ptr2++=*ptr; break;
			}
			ptr++;
		}
	}
	*ptr2=0;
	if (*ptr=='\"') ptr++;
	item->valuestring=out;
	item->type=cJSON_String;
	return ptr;
}

/* Render the cstring provided to an escaped version that can be printed. */
static int print_string_ptr(string_stream* ss, const char *str)
{
	const char *ptr;
	unsigned char token;
	
	if (!str) return cJSON_True;

	ptr=str;
	IfFailedReturn(push_char(ss, '\"'));
	while (*ptr)
	{
		if ((unsigned char)*ptr>31 && *ptr!='\"' && *ptr!='\\')
		{
			IfFailedReturn(push_char(ss, *ptr++));
		}
		else
		{
			IfFailedReturn(push_char(ss, '\\'));
			switch (token=*ptr++)
			{
				case '\\':	IfFailedReturn(push_char(ss, '\\')); break;
				case '\"':	IfFailedReturn(push_char(ss, '\"')); break;
				case '\b':	IfFailedReturn(push_char(ss, 'b')); break;
				case '\f':	IfFailedReturn(push_char(ss, 'f')); break;
				case '\n':	IfFailedReturn(push_char(ss, 'n')); break;
				case '\r':	IfFailedReturn(push_char(ss, 'r')); break;
				case '\t':	IfFailedReturn(push_char(ss, 't')); break;
				default: 
				{
					char buff[8] = {0};
					sprintf(buff,"u%04x",token);
					IfFailedReturn(push_string(ss, buff));
					break;	/* escape and print */
				}
			}
		}
	}
	IfFailedReturn(push_char(ss, '\"'));
	return cJSON_True;
}
/* Invote print_string_ptr (which is useful) on an item. */
static int print_string(string_stream* ss, cJSON *item)	{return print_string_ptr(ss, item->valuestring);}

/* Predeclare these prototypes. */
static const char *parse_value(cJSON *item,const char *value);
static int print_value(string_stream* ss,cJSON *item,int depth,int fmt);
static const char *parse_array(cJSON *item,const char *value);
static int print_array(cJSON *item,int depth,int fmt);
static const char *parse_object(cJSON *item,const char *value);
static int print_object(cJSON *item,int depth,int fmt);

/* Utility to jump whitespace and cr/lf */
static const char *skip(const char *in) {while (in && *in && (unsigned char)*in<=32) in++; return in;}

/* Parse an object - create a new root, and populate. */
cJSON *cJSON_ParseWithOpts(const char *value,const char **return_parse_end,int require_null_terminated)
{
	const char *end=0;
	cJSON *c=cJSON_New_Item();
	ep=0;
	if (!c) return 0;       /* memory fail */

	end=parse_value(c,skip(value));
	if (!end)	{cJSON_Delete(c);return 0;}	/* parse failure. ep is set. */

	/* if we require null-terminated JSON without appended garbage, skip and then check for a null terminator */
	if (require_null_terminated) {end=skip(end);if (*end) {cJSON_Delete(c);ep=end;return 0;}}
	if (return_parse_end) *return_parse_end=end;
	return c;
}
/* Default options for cJSON_Parse */
cJSON *cJSON_Parse(const char *value) {return cJSON_ParseWithOpts(value,0,0);}

/* Render a cJSON item/entity/structure to text. */
char *cJSON_Print(cJSON *item)				
{
	string_stream ss = {0};
	create_string_stream(&ss);

	if(print_value(&ss, item,0,1) == cJSON_True)
	{
		return ss.head;
	}
	cJSON_free(ss.head);

	return NULL;
}
char *cJSON_PrintUnformatted(cJSON *item)
{
	string_stream ss = {0};
	create_string_stream(&ss);

	if(print_value(&ss, item,0,0) == cJSON_True)
	{
		return ss.head;
	}
	cJSON_free(ss.head);

	return NULL;
}

/* Parser core - when encountering text, process appropriately. */
static const char *parse_value(cJSON *item,const char *value)
{
	if (!value)						return 0;	/* Fail on null. */
	if (!strncmp(value,"null",4))	{ item->type=cJSON_NULL;  return value+4; }
	if (!strncmp(value,"false",5))	{ item->type=cJSON_False; return value+5; }
	if (!strncmp(value,"true",4))	{ item->type=cJSON_True; item->valueint=1;	return value+4; }
	if (*value=='\"')				{ return parse_string(item,value); }
	if (*value=='-' || (*value>='0' && *value<='9'))	{ return parse_number(item,value); }
	if (*value=='[')				{ return parse_array(item,value); }
	if (*value=='{')				{ return parse_object(item,value); }

	ep=value;return 0;	/* failure. */
}

/* Render a value to text. */
static int print_value(string_stream* ss, cJSON *item,int depth,int fmt)
{
	int result = cJSON_False;

	if (!item) return 0;
	switch ((item->type)&255)
	{
		case cJSON_NULL:	result=push_string(ss, "null"); break;
		case cJSON_False:	result=push_string(ss, "false");break;
		case cJSON_True:	result=push_string(ss, "true"); break;
		case cJSON_Number:	result=print_number(ss, item);break;
		case cJSON_String:	result=print_string(ss, item);break;
		case cJSON_Array:	result=print_array(ss, item,depth,fmt);break;
		case cJSON_Object:	result=print_object(ss, item,depth,fmt);break;
	}
	
	return result;
}

/* Build an array from input text. */
static const char *parse_array(cJSON *item,const char *value)
{
	cJSON *child;
	if (*value!='[')	{ep=value;return 0;}	/* not an array! */

	item->type=cJSON_Array;
	item->arrayLength = 0;
	value=skip(value+1);
	if (*value==']') return value+1;	/* empty array. */

	item->child=child=cJSON_New_Item();
	if (!item->child) return 0;		 /* memory fail */
	value=skip(parse_value(child,skip(value)));	/* skip any spacing, get the value. */
	if (!value) return 0;
	item->arrayLength++;

	while (*value==',')
	{
		cJSON *new_item;
		if (!(new_item=cJSON_New_Item())) return 0; 	/* memory fail */
		child->next=new_item;new_item->prev=child;child=new_item;
		value=skip(parse_value(child,skip(value+1)));
		if (!value) return 0;	/* memory fail */
		item->arrayLength++;
	}

	if (*value==']') return value+1;	/* end of array */
	ep=value;return 0;	/* malformed. */
}

/* Render an array to text */
static int print_array(string_stream* ss, cJSON *item,int depth,int fmt)
{
	int j;
	cJSON *child=item->child;

	IfFailedReturn(push_char(ss, '['));

	/* Retrieve all the results: */
	while (child )
	{
		if(child->type == cJSON_Array)
		{
			IfFailedReturn(push_char(ss, '\n'));
			for(j = 0; j <= depth; j++)
			{
				IfFailedReturn(push_char(ss, '\t'));
			}
		}
		IfFailedReturn(print_value(ss, child,depth+1,fmt));
		child=child->next;
		if(child != NULL)
		{
			IfFailedReturn(push_char(ss, ','));
			if(fmt)
			{
				IfFailedReturn(push_char(ss, ' '));
			}
		}
	}

	IfFailedReturn(push_char(ss, ']'));

	return cJSON_True;
}

/* Build an object from the text. */
static const char *parse_object(cJSON *item,const char *value)
{
	cJSON *child;
	if (*value!='{')	{ep=value;return 0;}	/* not an object! */
	
	item->type=cJSON_Object;
	value=skip(value+1);
	if (*value=='}') return value+1;	/* empty array. */
	
	item->child=child=cJSON_New_Item();
	if (!item->child) return 0;
	value=skip(parse_string(child,skip(value)));
	if (!value) return 0;
	child->string=child->valuestring;child->valuestring=0;
	if (*value!=':') {ep=value;return 0;}	/* fail! */
	value=skip(parse_value(child,skip(value+1)));	/* skip any spacing, get the value. */
	if (!value) return 0;
	
	while (*value==',')
	{
		cJSON *new_item;
		if (!(new_item=cJSON_New_Item()))	return 0; /* memory fail */
		child->next=new_item;new_item->prev=child;child=new_item;
		value=skip(parse_string(child,skip(value+1)));
		if (!value) return 0;
		child->string=child->valuestring;child->valuestring=0;
		if (*value!=':') {ep=value;return 0;}	/* fail! */
		value=skip(parse_value(child,skip(value+1)));	/* skip any spacing, get the value. */
		if (!value) return 0;
	}
	
	if (*value=='}') return value+1;	/* end of array */
	ep=value;return 0;	/* malformed. */
}

/* Render an object to text. */
static int print_object(string_stream* ss, cJSON *item,int depth,int fmt)
{
	cJSON *child=item->child;
	int i;

	/* Explicitly handle empty object case */
	if (child == NULL)
	{
		IfFailedReturn(push_char(ss, '{'));
		if(fmt)
		{
			IfFailedReturn(push_char(ss, '\n'));
			for(i = 0; i < depth-1; i++)
			{
				IfFailedReturn(push_char(ss, '\t'));
			}
		}

		return cJSON_True;
	}

	/* Collect all the results into our arrays: */
	depth++;

	IfFailedReturn(push_char(ss, '{'));
	if(fmt)
	{
		IfFailedReturn(push_char(ss, '\n'));
	}

	while (child)
	{
		if(fmt)
		{
			for(i = 0; i < depth; i++)
			{
				IfFailedReturn(push_char(ss, '\t'));
			}
		}
		IfFailedReturn(print_string_ptr(ss, child->string));
		if(fmt)
		{
			IfFailedReturn(push_char(ss, ' '));
		}
		IfFailedReturn(push_char(ss, ':'));
		if(fmt)
		{
			IfFailedReturn(push_char(ss, ' '));
		}
		IfFailedReturn(print_value(ss, child, depth, fmt));
		child=child->next;

		if(child != NULL)
		{
			IfFailedReturn(push_char(ss, ','));
		}
		if(fmt)
		{
			IfFailedReturn(push_char(ss, '\n'));
		}
	}

	if(fmt)
	{
		for(i = 0; i < depth-1; i++)
		{
			IfFailedReturn(push_char(ss, '\t'));
		}
	}
	IfFailedReturn(push_char(ss, '}'));

	return cJSON_True;
}

/* Get Array size/item / object item. */
int    cJSON_GetArraySize(cJSON *array)							{return array->arrayLength;}
cJSON *cJSON_GetArrayItem(cJSON *array,int item)				{cJSON *c=array->child;  while (c && item>0) item--,c=c->next; return c;}
cJSON *cJSON_GetObjectItem(cJSON *object,const char *string)	{cJSON *c=object->child; while (c && cJSON_strcasecmp(c->string,string)) c=c->next; return c;}

/* Array Iterator functions */
int    cJSON_ArrayIteratorMoveNext(cJSON* it)
{
	if(it && it->type == cJSON_ArrayIterator)
	{
		if(it->child)
		{
			it->prev = it->child;
			it->child = it->next;
		}
		else if(it->next)
		{
			it->child = it->next;
		}
		else
		{
			return cJSON_False;
		}

		if(it->child->next)
		{
			it->next = it->child->next;
		}

		return cJSON_True;
	}
	return cJSON_False;
}
int    cJSON_ArrayIteratorMovePrev(cJSON* it)
{
	if(it && it->type == cJSON_ArrayIterator)
	{
		if(it->child)
		{
			it->next = it->child;
			it->child = it->prev;
		}
		else if(it->prev)
		{
			it->child = it->prev;
		}
		else
		{
			return cJSON_False;
		}

		if(it->child->prev)
		{
			it->prev = it->child->prev;
		}

		return cJSON_True;
	}
	return cJSON_False;
}
cJSON *cJSON_ArrayIteratorCurrent(cJSON* it)
{
	if(it->type == cJSON_ArrayIterator)
	{
		return it->child;
	}
	return 0;
}


/* Utility for array list handling. */
static void suffix_object(cJSON *prev,cJSON *item) {prev->next=item;item->prev=prev;}
/* Utility for handling references. */
static cJSON *create_reference(cJSON *item) {cJSON *ref=cJSON_New_Item();if (!ref) return 0;memcpy(ref,item,sizeof(cJSON));ref->string=0;ref->type|=cJSON_IsReference;ref->next=ref->prev=0;return ref;}

/* Add item to array/object. */
void   cJSON_AddItemToArray(cJSON *array, cJSON *item)						{cJSON *c=array->child;if (!item) return; if (!c) {array->child=item;} else {while (c && c->next) c=c->next; suffix_object(c,item);}array->arrayLength++;}
void   cJSON_AddItemToObject(cJSON *object,const char *string,cJSON *item)	{if (!item) return; if (item->string) cJSON_free(item->string);item->string=cJSON_strdup(string);cJSON_AddItemToArray(object,item);}
void	cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item)						{cJSON_AddItemToArray(array,create_reference(item));}
void	cJSON_AddItemReferenceToObject(cJSON *object,const char *string,cJSON *item)	{cJSON_AddItemToObject(object,string,create_reference(item));}

cJSON *cJSON_DetachItemFromArray(cJSON *array,int which)			{cJSON *c=array->child;while (c && which>0) c=c->next,which--;if (!c) return 0;
	if (c->prev) c->prev->next=c->next;if (c->next) c->next->prev=c->prev;if (c==array->child) array->child=c->next;c->prev=c->next=0;array->arrayLength--;return c;}
void   cJSON_DeleteItemFromArray(cJSON *array,int which)			{cJSON_Delete(cJSON_DetachItemFromArray(array,which));}
cJSON *cJSON_DetachItemFromObject(cJSON *object,const char *string) {int i=0;cJSON *c=object->child;while (c && cJSON_strcasecmp(c->string,string)) i++,c=c->next;if (c) return cJSON_DetachItemFromArray(object,i);return 0;}
void   cJSON_DeleteItemFromObject(cJSON *object,const char *string) {cJSON_Delete(cJSON_DetachItemFromObject(object,string));}

/* Replace array/object items with new ones. */
void   cJSON_ReplaceItemInArray(cJSON *array,int which,cJSON *newitem)		{cJSON *c=array->child;while (c && which>0) c=c->next,which--;if (!c) return;
	newitem->next=c->next;newitem->prev=c->prev;if (newitem->next) newitem->next->prev=newitem;
	if (c==array->child) array->child=newitem; else newitem->prev->next=newitem;c->next=c->prev=0;cJSON_Delete(c);}
void   cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem){int i=0;cJSON *c=object->child;while(c && cJSON_strcasecmp(c->string,string))i++,c=c->next;if(c){newitem->string=cJSON_strdup(string);cJSON_ReplaceItemInArray(object,i,newitem);}}

/* Create basic types: */
cJSON *cJSON_CreateNull(void)					{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_NULL;return item;}
cJSON *cJSON_CreateTrue(void)					{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_True;return item;}
cJSON *cJSON_CreateFalse(void)					{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_False;return item;}
cJSON *cJSON_CreateBool(int b)					{cJSON *item=cJSON_New_Item();if(item)item->type=b?cJSON_True:cJSON_False;return item;}
cJSON *cJSON_CreateNumber(double num)			{cJSON *item=cJSON_New_Item();if(item){item->type=cJSON_Number;item->valuedouble=num;item->valueint=(int)num;}return item;}
cJSON *cJSON_CreateString(const char *string)	{cJSON *item=cJSON_New_Item();if(item){item->type=cJSON_String;item->valuestring=cJSON_strdup(string);}return item;}
cJSON *cJSON_CreateArray(void)
{
	cJSON *item=cJSON_New_Item();
	if(item)
	{
		item->type=cJSON_Array;
		item->arrayLength = 0;
	}
	return item;
}
cJSON *cJSON_CreateObject(void)					{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_Object;return item;}


/* Create Arrays: */
cJSON *cJSON_CreateIntArray(int *numbers,int count)				{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}a->arrayLength=count;return a;}
cJSON *cJSON_CreateFloatArray(float *numbers,int count)			{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}a->arrayLength=count;return a;}
cJSON *cJSON_CreateDoubleArray(double *numbers,int count)		{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}a->arrayLength=count;return a;}
cJSON *cJSON_CreateStringArray(const char **strings,int count)	{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateString(strings[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}a->arrayLength=count;return a;}
cJSON *cJSON_CreateArrayIterator(cJSON* array)
{
	cJSON* item = array->type == cJSON_Array ? cJSON_New_Item() : 0;
	if(item) 
	{	
		item->type = cJSON_ArrayIterator;
		item->child = 0;
		item->prev = 0;
		item->next = array->child;
	}
	return item;
}

/* Duplication */
cJSON *cJSON_Duplicate(cJSON *item,int recurse)
{
	cJSON *newitem,*cptr,*nptr=0,*newchild;
	/* Bail on bad ptr */
	if (!item) return 0;
	/* Create new item */
	newitem=cJSON_New_Item();
	if (!newitem) return 0;
	/* Copy over all vars */
	newitem->type=item->type&(~cJSON_IsReference),newitem->valueint=item->valueint,newitem->valuedouble=item->valuedouble;
	if (item->valuestring)	{newitem->valuestring=cJSON_strdup(item->valuestring);	if (!newitem->valuestring)	{cJSON_Delete(newitem);return 0;}}
	if (item->string)		{newitem->string=cJSON_strdup(item->string);			if (!newitem->string)		{cJSON_Delete(newitem);return 0;}}
	/* If non-recursive, then we're done! */
	if (!recurse) return newitem;
	/* Walk the ->next chain for the child. */
	cptr=item->child;
	while (cptr)
	{
		newchild=cJSON_Duplicate(cptr,1);		/* Duplicate (with recurse) each item in the ->next chain */
		if (!newchild) {cJSON_Delete(newitem);return 0;}
		if (nptr)	{nptr->next=newchild,newchild->prev=nptr;nptr=newchild;}	/* If newitem->child already set, then crosswire ->prev and ->next and move on */
		else		{newitem->child=newchild;nptr=newchild;}					/* Set newitem->child and move to it */
		cptr=cptr->next;
	}
	return newitem;
}
