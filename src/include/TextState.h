
#ifndef TEXTSTATE_H__
#define TEXTSTATE_H__

#include <string>

namespace pdf2htmlEX {

class TextState {
public:
	void begin(std::ostream & out, const TextState * prev_state);
	void end(std::ostream & out) const;
	void hash(void);
	int diff(const TextState & s) const;

	enum {
		FONT_ID,
		FONT_SIZE_ID,
		COLOR_ID,
		LETTER_SPACE_ID,
		WORD_SPACE_ID,
		RISE_ID,

		ID_COUNT
	};

	long long ids[ID_COUNT];

	double ascent;
	double descent;
	double draw_font_size;

	size_t start_idx; // index of the first Text using this state
	// for optimzation
	long long hash_value;
	bool need_close;

	static const char * format_str; // class names for each id
};


} // namespace pdf2htmlEX

#endif //TEXTSTATE_h__
