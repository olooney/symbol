// these private utilities are exposed here ONLY for the purposes of isolation testing.
// they are not fit for any other purpose.
namespace symbol {
	bool isHex(const char);
	bool isIdentifier(const char);
	bool matchesHashedFormat(const char* word);
}
