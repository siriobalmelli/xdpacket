/*	parser_test.c
 * (c) 2018 Sirio Balmelli
 * (c) 2018 Alan Morrissett
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <getopt.h>
#include <assert.h>
#include <yaml.h>
#include <parse.h>

/*	field_check()
 * Validate that field hashing matches known values.
 */
int bulk_parse_check()
{
	int err_cnt = 0;

	return err_cnt;
}

#ifdef BUF_SIZE
#undef BUF_SIZE
#endif
#define BUF_SIZE 4096

/*	interactive_parse_check()
 */
void interactive_parse_check()
{
	char buf[BUF_SIZE];
	size_t written = 0;
	yaml_parser_t parser;
	yaml_document_t document;
	yaml_emitter_t emitter;

	while (true) {
		int len = 0;
		memset(buf, 0, BUF_SIZE);
		printf("Enter YAML: ");
		if (fgets(buf, sizeof(buf), stdin) == NULL)
			break;
		len = strlen(buf);
		if (buf[len-1] == '\n')
			buf[--len] = '\0';
		if (len == 0) 
			continue;
		printf("You entered: '%s'\n", buf);

		yaml_parser_initialize(&parser);
		yaml_parser_set_input_string(&parser, (unsigned char*)buf, len);
		if (!yaml_parser_load(&parser, &document)) {  /* returns 1 on success */
			yaml_mark_t *mark = &parser.context_mark;
			fprintf(stderr, "parse failed: %s\n", parser.problem);
			fprintf(stderr, "%s\n", buf);
			for (int i = 0; i <= mark->column; i++) 
				fprintf(stderr, " ");
			fprintf(stderr, "^\n");
			continue;
		}
		printf("depth == %d\n", tree_depth(&document, yaml_document_get_root_node(&document)));

		/**/
		yaml_node_t *node;
		for (node = document.nodes.start; node < document.nodes.top; node++) {
			switch (node->type) {
			case YAML_SCALAR_NODE:
				printf("Scalar node: %s\n", node->data.scalar.value);
				break;
			case YAML_SEQUENCE_NODE:
				printf("Sequence node: %s\n", node->tag);
				node->data.sequence.style = YAML_BLOCK_SEQUENCE_STYLE;
				break;
			case YAML_MAPPING_NODE:
				printf("Mapping node: %s\n", node->tag);
				node->data.mapping.style = YAML_BLOCK_MAPPING_STYLE;
				break;
			default:
				assert(0);
				break;
			}
		}
		/* renders YAML to stdout */
		print_yaml_document(&document);

		memset(buf, 0, BUF_SIZE);
		yaml_emitter_initialize(&emitter);
		yaml_emitter_set_unicode(&emitter, 1);
		yaml_emitter_set_output_string(&emitter, (yaml_char_t*)buf, BUF_SIZE, &written);
		yaml_emitter_dump(&emitter, &document);
		yaml_emitter_flush(&emitter);
		printf("%s\n", buf);		

		yaml_parser_delete(&parser);
		yaml_emitter_close(&emitter);
		yaml_emitter_delete(&emitter);
		yaml_document_delete(&document);
	}
}

/*	main()
 * Run all test routines in this file.
 * Expects test routines to return err_cnt (0 on success).
 */
int main(int argc, char *argv[])
{
	int err_cnt = 0;
	int opt = 0;
	bool interactive = false;
	bool parse_check = false;

	if (isatty(0)) {
		while ((opt = getopt(argc, argv, "p")) != -1) {
			interactive = true;
			switch (opt) {
			case 'p':
				parse_check = true;
				break;
			default:
				fprintf(stderr, "Usage: %s [-n]\n", argv[0]);
				exit(EXIT_FAILURE);
			}
		}
		if (interactive)
			interactive_parse_check();
	}

	if (!interactive)
		err_cnt += bulk_parse_check();

	return err_cnt;
}
