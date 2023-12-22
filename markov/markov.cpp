#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>

typedef unsigned long hash;

// Word characters.
bool is_word_char(char c) {
	return isalpha(c) || c == '-' || c == '_' || c == '\'';
}

bool is_punc_word_char(char c) {
	return c == ',' || c == ';' || c == ':' || c == '.' || c == '!' || c == '?';
}

// djb2 hash function
// http://www.cse.yorku.ca/~oz/hash.html
// Writes number of characters consumed to the argument.
hash djb2(const unsigned char *str, int limit=65535, int* chars_consumed=nullptr) {
	const unsigned char* str_start = str;
	unsigned long hash = 5381;

	for (; (is_word_char(*str) || is_punc_word_char(*str)) && str - str_start < limit; str++) {
		hash = ((hash << 5) + hash) + *str; // hash * 33 + *str
	}

	if (chars_consumed != nullptr) {
		*chars_consumed = str - str_start;
	}
	return hash;
}

// Compares strings until a newline or a null-terminating character is reached.
// Returns true if they are equal.
bool str_eq_lf(const char* a, const char* b, int limit=65535) {
	for (int i = 0; !((a[i] == '\n' && b[i] == '\n') || (a[i] == '\0' && b[i] != '\0')) && i < limit; i++) if (a[i] != b[i]) return false;
	return true;
}

// Returns the length of a string terminated by either a null-terminating character or a newline.
int strlen_lf(const char* a) {
	int i;
	for (i = 0; a[i] != '\n' && a[i] != '\0'; i++);
	return i;
}

void str_prn_lf(const char* a, int limit=65535) {
	for (int c = 0; a[c] != '\0' && a[c] != '\n' && c < limit; c++) printf("%c", a[c]);
}

class markov {
public:
	char* name;
	char* text;
	int text_cur_len;
	
	unsigned long* token_text; // Array of tokens corresponding to words.
	int* token_text_ind; // Index of the word identified by the token at index.
	int* token_text_len; // Length of the word identified by the token at index.
	int num_tokens;
	
	int num_unique_tokens;
	
	// graph[i][j] gives the probability that the word i is followed by the word j, where i and j are token IDs.
	float** graph;
	
	markov() :
		name(nullptr), text(nullptr), text_cur_len(0),
		token_text(nullptr), token_text_ind(nullptr), token_text_len(nullptr), num_tokens(0),
		num_unique_tokens()
	{}
	
	void set_name(const char* markov_name, int name_len) {
		name = new char[name_len+1];
		memcpy(name, markov_name, name_len);
		name[name_len] = '\0';
	}
	
	void append_text(const char* new_text, int new_text_len) {
		text = (char*) realloc(text, text_cur_len + new_text_len + 2);
		
		text[text_cur_len] = ' ';
		memcpy(text+text_cur_len+1, new_text, new_text_len);
		
		text_cur_len += new_text_len + 1;
		text[text_cur_len] = '\0';
	}
	
	void tokenize() {
		printf("Tokenizing %s... ", name);
		
		// Count words.
		int num_words = 0;
		int start = -1;
		for (int c = 0; c < text_cur_len; c++) {
			if (is_punc_word_char(text[c])) {
				if (start != -1) {
					num_words++;
				}
				
				num_words++;
				start = -1;
			}
			else if (is_word_char(text[c]) && start == -1) {
				start = c;
			}
			else if (!is_word_char(text[c]) && start != -1) {
				num_words++;
				start = -1;
			}
		}
		
		printf("Found %d words... ", num_words);
		
		num_tokens = num_words;
		token_text = new hash[num_tokens];
		token_text_ind = new int[num_tokens];
		token_text_len = new int[num_tokens];
		
		num_words = 0;
		start = -1;
		for (int c = 0; c < text_cur_len; c++) {
			if (is_punc_word_char(text[c])) {
				if (start != -1) {
					token_text[num_words] = djb2((unsigned char*) text + start, c - start);
					token_text_ind[num_words] = start;
					token_text_len[num_words] = c - start;
					
					// printf("%d (", num_words);
					// str_prn_lf(text + token_text_ind[num_words], token_text_len[num_words]);
					// printf(") -> (%ul)\n", token_text[num_words]);
					
					num_words++;
				}
				
				token_text[num_words] = djb2((unsigned char*) text + c, 1);
				token_text_ind[num_words] = c;
				token_text_len[num_words] = 1;
					
				// printf("%d (", num_words);
				// str_prn_lf(text + token_text_ind[num_words], token_text_len[num_words]);
				// printf(") -> (%ul)\n", token_text[num_words]);
				
				num_words++;
				start = -1;
			}
			else if (is_word_char(text[c]) && start == -1) {
				start = c;
			}
			else if (!is_word_char(text[c]) && start != -1) {
				token_text[num_words] = djb2((unsigned char*) text + start, c - start);
				token_text_ind[num_words] = start;
				token_text_len[num_words] = c - start;
					
				// printf("%d (", num_words);
				// str_prn_lf(text + token_text_ind[num_words], token_text_len[num_words]);
				// printf(") -> (%ul)\n", token_text[num_words]);
				
				num_words++;
				start = -1;
			}
		}
		
		// Convert hash strings to tokens and count unique words.
		
		num_unique_tokens = 1;
		hash* unique_tokens = new hash[num_tokens]; // Running list of unique hashes.
		unique_tokens[0] = djb2((const unsigned char*) ".", 1);
		
		for (int i = 0; i < num_tokens; i++) {
			// printf("%d (", i);
			// str_prn_lf(text + token_text_ind[i], token_text_len[i]);
			// printf(") ");
			bool is_new_unique = true;
			for (int j = 0; j < num_unique_tokens; j++) {
				if (token_text[i] == unique_tokens[j]) {
					// TODO: Add check for equality between underlying strings.
					token_text[i] = j;
					// printf("= %d\n", j);
					is_new_unique = false;
					break;
				}
			}
			
			if (is_new_unique) {
				// printf("is new.\n");
				unique_tokens[num_unique_tokens] = token_text[i];
				token_text[i] = num_unique_tokens;
				num_unique_tokens++;
			}
		}
		
		printf("%d unique. Saturation %.2f%%.\n", num_unique_tokens, (float) num_tokens / (num_unique_tokens*num_unique_tokens) * 100);
		
		delete[] unique_tokens;
	}
	
	void generate_graph() {
		printf("Graphing for %s... \n", name);
		
		// for (int i = 0; i < num_tokens; i++) {
			// printf("%d: \"", token_text[i]);
			// str_prn_lf(text + token_text_ind[i], token_text_len[i]);
			// printf("\"\n");
		// }
		// printf("\n");
		
		graph = new float*[num_unique_tokens];
		for (int i = 0; i < num_unique_tokens; i++) {
			graph[i] = new float[num_unique_tokens];
			for (int j = 0; j < num_unique_tokens; j++) {
				graph[i][j] = 0;
			}
		}
		
		for (int i = 0; i < num_tokens-1; i++) {
			int tkn_a = token_text[i];
			int tkn_b = token_text[i+1];
			graph[tkn_a][tkn_b]++;
		}
		
		for (int i = 0; i < num_unique_tokens; i++) {
			int sum = 0;
			for (int j = 0; j < num_unique_tokens; j++) {
				sum += graph[i][j];
			}
			for (int j = 0; j < num_unique_tokens; j++) {
				graph[i][j] /= sum;
			}
		}
		
		for (int i = 0; i < num_unique_tokens; i++) {
			for (int j = 0; j < num_unique_tokens; j++) {
				printf("%4.2f ", graph[i][j]);
			}
			printf("\n");
		}
		printf("\n");
	}
	
	void generate_text(int max_words) {
		
	
	~markov() {
		if (name != nullptr) {
			delete[] name;
		}
		
		if (text != nullptr) {
			free(text);
		}
		
		if (token_text != nullptr) {
			delete[] token_text;
		}
		
		if (token_text_ind != nullptr) {
			delete[] token_text_ind;
		}
		
		if (token_text_len != nullptr) {
			delete[] token_text_len;
		}
		
		if (graph != nullptr) {
			for (int i = 0; i < num_unique_tokens; i++) {
				delete[] graph[i];
			}
			delete[] graph;
		}
	}
};

int main(int argc, const char** argv) {
	{
		const char* in_fn;
		const char* out_fn;
		if (argc < 2 || argc > 3) {
			printf("Usage:\nmarkov <input file> [output file]\n");
			exit(1);
		}
		else {
			in_fn = argv[1];
			if (argc == 3) {
				out_fn = argv[2];
			}
			else {
				out_fn = "markov_chains.json";
			}
		}
		
		printf("%s -> %s\n", in_fn, out_fn);
		
		// Load data
		FILE* fin = fopen(in_fn, "r");
		if (fin == nullptr) {
			printf("Failed to open file.");
			exit(1);
		}
		
		fseek(fin, 0, SEEK_END);
		int file_size = ftell(fin);
		rewind(fin);
		
		char* text = new char[file_size+1];
		int bytes_read = fread(text, 1, file_size, fin);
		fclose(fin);
		
		text[bytes_read] = '\0';
		
		for (int c = 0; c < bytes_read; c++) {
			text[c] = tolower(text[c]); // Lowercase all text.
		}
		
		// Count speakers.
		int num_speakers = 0;
		bool on_new_line = true;
		for (int c = 0; c < bytes_read; c++) {
			if (text[c] == '\n') {
				on_new_line = true;
			}
			else {
				if (text[c] == '>') {
					num_speakers++;
					on_new_line = false;
				}
				on_new_line = false;
			}
		}
		printf("Found %d Speaker lines.\n", num_speakers);
		
		char** speakers = new char*[num_speakers];
		int* speakers_len = new int[num_speakers];
		
		on_new_line = true;
		bool preserve_newline = false;
		int num_speakers_written = 0;
		for (int c = 0; c < bytes_read; c++) {
			if (on_new_line && text[c] == '>') {
				if (c > 0) text[c-1] = '\n';
				preserve_newline = true;
				on_new_line = false;
				
				speakers[num_speakers_written] = text + c + 1;
				speakers_len[num_speakers_written] = strlen_lf(text + c + 1);
				num_speakers_written++;
			}
			else if (text[c] == '\n') {
				if (preserve_newline) {
					preserve_newline = false;
				}
				else {
					text[c] = ' ';
				}
				on_new_line = true;
			}
			else {
				on_new_line = false;
			}
		}
		
		// Trim whitespace from speaker names.
		for (int i = 0; i < num_speakers; i++) {
			int first_char = -1;
			int last_char = -1;
			
			for (int c = 0; c < speakers_len[i]; c++) {
				if (isalpha(speakers[i][c])) {
					first_char = c;
					break;
				}
			}
			
			for (int c = speakers_len[i] - 1; c >= 0; c--) {
				if (isalpha(speakers[i][c])) {
					last_char = c;
					break;
				}
			}
			
			speakers[i] += first_char;
			speakers_len[i] = last_char - first_char + 1;
		}
		
		printf("Indexed %d speakers.\n", num_speakers_written);
		
		// Catalogue distinct speakers and the number of lines they have.
		int* lines_per_unique_speaker = new int[num_speakers];
		int* unique_speakers_ind = new int[num_speakers];
		int num_unique_speakers = 0;
		for (int i = 0; i < num_speakers; i++) {
			bool do_add_speaker = true;
			for (int j = 0; j < num_unique_speakers; j++) {
				if (str_eq_lf(speakers[i], speakers[unique_speakers_ind[j]], speakers_len[i])) {
					do_add_speaker = false;
					lines_per_unique_speaker[j]++;
					break;
				}
			}
			
			if (do_add_speaker) {
				unique_speakers_ind[num_unique_speakers] = i;
				lines_per_unique_speaker[num_unique_speakers] = 1;
				num_unique_speakers++;
			}
		}
		
		printf("Found %d unique speakers.\n", num_unique_speakers);
		
		// Load speaker names into array.
		markov* markovs = new markov[num_unique_speakers];
		for (int i = 0; i < num_unique_speakers; i++) {
			markovs[i].set_name(speakers[unique_speakers_ind[i]], speakers_len[unique_speakers_ind[i]]);
		}
		
		// Load speaker lines into array.
		for (int i = 0; i < num_speakers; i++) {
			char* c;
			char* d;
			
			// printf("%d: %p: %p + %d\n", i, text, speakers[i], speakers_len[i]);
			
			for (c = speakers[i] + speakers_len[i]; *c != '\n'; c++);
			c++;
			
			for (d = c; *d != '\n'; d++);
			
			// str_prn_lf(c);
			// printf("\n");
			
			for (int j = 0; j < num_unique_speakers; j++) {
				if (str_eq_lf(speakers[i], speakers[unique_speakers_ind[j]], speakers_len[i])) {
					markovs[j].append_text(c, d - c);
				}
			}
		}
		
		delete[] text;
		
		// Text is now loaded into markov instances.
		
		//markovs[2].tokenize();
		
		for (int i = 0; i < num_unique_speakers; i++) {
			markovs[i].tokenize();
		}
		
		markovs[2].generate_graph();
		
		// for (int i = 0; i < num_unique_speakers; i++) {
			// markovs[i].generate_graph();
		// }
		
		//printf("%s", text);
		delete[] speakers;
		delete[] speakers_len;
		
		delete[] unique_speakers_ind;
		delete[] lines_per_unique_speaker;
		
		delete[] markovs;
	}
	printf("Done.\n");
	return 0;
}