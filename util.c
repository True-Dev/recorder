#include <stdio.h>
#include <stdlib.h>
#include "util.h"

const char *isotime(time_t t) {
        static char buf[] = "YYYY-MM-DDTHH:MM:SSZ";

        strftime(buf, sizeof(buf), "%FT%TZ", gmtime(&t));
        return(buf);
}

char *slurp_file(char *filename, int fold_newlines)
{
	FILE *fp;
	char *buf, *bp;
	off_t len;
	int ch;

	if ((fp = fopen(filename, "rb")) == NULL)
		return (NULL);

	if (fseeko(fp, 0, SEEK_END) != 0) {
		fclose(fp);
		return (NULL);
	}
	len = ftello(fp);
	fseeko(fp, 0, SEEK_SET);

	if ((bp = buf = malloc(len + 1)) == NULL) {
		fclose(fp);
		return (NULL);
	}
	while ((ch = fgetc(fp)) != EOF) {
		if (ch == '\n') {
			if (!fold_newlines)
				*bp++ = ch;
		} else *bp++ = ch;
	}
	*bp = 0;
	fclose(fp);

	return (buf);
}

int json_copy_to_object(JsonNode * obj, JsonNode * object_or_array)
{
	JsonNode *node;

	if (obj->tag != JSON_OBJECT && obj->tag != JSON_ARRAY)
		return (FALSE);

	/* should we delete keys which already exist in `obj' ? */
	json_foreach(node, object_or_array) {
		if (obj->tag == JSON_OBJECT) {
			if (node->tag == JSON_STRING)
				json_append_member(obj, node->key, json_mkstring(node->string_));
			else if (node->tag == JSON_NUMBER)
				json_append_member(obj, node->key, json_mknumber(node->number_));
			else if (node->tag == JSON_BOOL)
				json_append_member(obj, node->key, json_mkbool(node->bool_));
			else if (node->tag == JSON_NULL)
				json_append_member(obj, node->key, json_mknull());
			else if (node->tag == JSON_ARRAY) {
				JsonNode       *array = json_mkarray();
				json_copy_to_object(array, node);
				json_append_member(obj, node->key, array);
			} else if (node->tag == JSON_OBJECT) {
				JsonNode       *newobj = json_mkobject();
				json_copy_to_object(newobj, node);
				json_append_member(obj, node->key, newobj);
			} else
				printf("PANIC: unhandled JSON type %d\n", node->tag);
		} else if (obj->tag == JSON_ARRAY) {
			if (node->tag == JSON_STRING)
				json_append_element(obj, json_mkstring(node->string_));
			if (node->tag == JSON_NUMBER)
				json_append_element(obj, json_mknumber(node->number_));
			if (node->tag == JSON_BOOL)
				json_append_element(obj, json_mkbool(node->bool_));
			if (node->tag == JSON_NULL)
				json_append_element(obj, json_mknull());
		}
	}
	return (TRUE);
}

/*
 * Open filename for reading; slurp in the whole file and attempt
 * to decode JSON from it into the JSON object at `obj'. TRUE on success, FALSE on failure.
 */

int json_copy_from_file(JsonNode *obj, char *filename)
{
	char *js_string;
	JsonNode *node;

	if ((js_string = slurp_file(filename, TRUE)) != NULL) {
		if ((node = json_decode(js_string)) == NULL) {
			fprintf(stderr, "json_copy_from_file can't decode JSON from %s\n", filename);
			return (FALSE);
		}
		json_copy_to_object(obj, node);
		json_delete(node);

		free(js_string);
	}
	return (TRUE);
}