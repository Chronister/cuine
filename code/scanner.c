#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "scanner.h"

static int
IsDigit(int C) {
    return '0' <= C && C <= '9';
}

static int
IsWordCharacter(int C) {
    return ('a' <= C && C <= 'z') ||
           ('A' <= C && C <= 'Z') ||
           (C == '_') || (C == '$');
}

static bool
IsKeyword(const char* Keyword, const char* Text, int TextLength) {
    int KeywordLength = strlen(Keyword);
    if (KeywordLength != TextLength) { return false; }
    int Length = TextLength;
    for (int i = 0; i < Length; ++i) {
        if (Keyword[i] != Text[i]) return false;
    }
    return true;
}

static token_type
GetKeywordType(token Token)
{
    for (int i = AUTO; i <= _IMAGINARY; ++i) {
        if (IsKeyword(TokenType_Str(i), Token.Text, Token.TextLength)) return (token_type)i;
    }
    return Identifier;
}

static void
ReportScanError(tokenizer* Tzer, const char* Fmt, ...)
{
    va_list Args;
    va_start(Args, Fmt);
    // Do thing
    fprintf(stderr, "%s:%d:"PRIdPTR": ", Tzer->Filename, Tzer->Line, Tzer->At - Tzer->LineStart);
    vfprintf(stderr, Fmt, Args);
    fprintf(stderr, "\n");
    return;
}

static void
EatAllWhitespace(tokenizer* Tzer)
{
    for(;;)
    {
        switch(*Tzer->At) {
            case '\r':
            case '\f':
            case ' ': { Tzer->At++; } break;
            case '\n': 
            { 
                Tzer->Line++; Tzer->At++; Tzer->LineStart = Tzer->At; 
            } break;

            case '/':
            {
                switch(*(Tzer->At + 1)) {
                    case '/':
                        // TODO this isn't quite right, newline can be escaped
                        while (*Tzer->At && *Tzer->At != '\n') { 
                            Tzer->At++; 
                        }
                        break;

                    case '*':
                        Tzer->At++;
                        Tzer->At++;
                        while (*Tzer->At && !(*Tzer->At == '*' && *Tzer->At == '/')) { 
                            Tzer->At++; 
                        }
                        break;

                    default: return;
                }
            } break;

            // Ignore preprocessor for now
            // TODO this isn't quite right, newline can be escaped
            case '#':
            {
                while (*Tzer->At && *Tzer->At != '\n') { 
                    Tzer->At++; 
                }
                break;
            } break;

            default: return;
        }
    }
}

extern const char*
TokenType_Str(token_type Type)
{
    int T = (int)Type;
    if (T >= 0 && T < C_NonterminalMAX) {
        return C_SymbolNames[Type];
    }
    return "??? Unknown Token ???";
}

extern token 
GetToken(tokenizer* Tzer) 
{
    token Token = {0};

    // State machine for parsing tokens.
    // Liberal use of switch and goto used for simplifying the control flow.
get_token:
    EatAllWhitespace(Tzer);

    Token.LineStart = Token.LineEnd = Tzer->Line;

    switch (*Tzer->At) {
        // EOF
        case '\0': Token.Type = EndOfFile; return Token;

        // Grouping symbols
        case '(': Token.Type = LParen; Tzer->At++; return Token;
        case ')': Token.Type = RParen; Tzer->At++; return Token;
        case '[': Token.Type = LBracket; Tzer->At++; return Token;
        case ']': Token.Type = RBracket; Tzer->At++; return Token;
        case '{': Token.Type = LCurly; Tzer->At++; return Token;
        case '}': Token.Type = RCurly; Tzer->At++; return Token;

        // Separators
        case ',': Token.Type = Comma; Tzer->At++; return Token;
        case ';': Token.Type = Semicolon; Tzer->At++; return Token;
        case ':': Token.Type = Colon; Tzer->At++; return Token;

        case '.': {
            Tzer->At++;
            if (IsDigit(*Tzer->At)) 
                goto float_scan;
            if (*Tzer->At == '.') {
                if (*(Tzer->At + 1) == '.') {
                    Tzer->At += 2;
                    Token.Type = Ellipses;
                    return Token;
                }
            }
            Token.Type = Dot; 
            return Token;
        } break;

        // Operators
        case '=': 
            ++Tzer->At;
            switch (*Tzer->At) {
                case '=': Token.Type = Equals; ++Tzer->At; return Token;
                default:  Token.Type = Assign; return Token;
            }
            break;

        case '+': 
            ++Tzer->At;
            switch (*Tzer->At) {
                case '=': Token.Type = PlusEquals; ++Tzer->At; return Token;
                case '+': Token.Type = Increment; ++Tzer->At; return Token;
                default:  Token.Type = Plus; return Token;
            }
            break;
        case '-': 
            ++Tzer->At;
            switch (*Tzer->At) {
                case '>': Token.Type = Arrow; ++Tzer->At; return Token;
                case '=': Token.Type = MinusEquals; ++Tzer->At; return Token;
                case '-': Token.Type = Decrement; ++Tzer->At; return Token;
                default:  Token.Type = Minus; return Token;
            }
            break;
        case '*': 
            ++Tzer->At;
            switch (*Tzer->At) {
                case '=': Token.Type = TimesEquals; ++Tzer->At; return Token;
                default:  Token.Type = Asterisk; return Token;
            }
            break;
        case '/': 
            ++Tzer->At;
            switch (*Tzer->At) {
                case '=': Token.Type = DivideEquals; ++Tzer->At; return Token;
                default:  Token.Type = Divide; return Token;
            }
            break;
            
        case '!': 
            ++Tzer->At;
            switch (*Tzer->At) {
                case '=': Token.Type = LogicNotEquals; ++Tzer->At; return Token;
                default:  Token.Type = LogicNot; return Token;
            }
            break;
        case '~': 
            ++Tzer->At;
            switch (*Tzer->At) {
                case '=': Token.Type = BitNotEquals; ++Tzer->At; return Token;
                default:  Token.Type = BitNot; return Token;
            }
            break;
        case '^': 
            ++Tzer->At;
            switch (*Tzer->At) {
                case '=': Token.Type = BitXorEquals; ++Tzer->At; return Token;
                default:  Token.Type = BitXor; return Token;
            }
            break;

        case '&': 
            ++Tzer->At;
            switch (*Tzer->At) {
                case '=': Token.Type = BitAndEquals; ++Tzer->At; return Token;
                case '&': Token.Type = LogicAnd; ++Tzer->At; return Token;
                default:  Token.Type = Ampersand; return Token;
            }
            break;
        case '|': 
            ++Tzer->At;
            switch (*Tzer->At) {
                case '=': Token.Type = BitOrEquals; ++Tzer->At; return Token;
                case '|': Token.Type = LogicOr; ++Tzer->At; return Token;
                default:  Token.Type = BitOr; return Token;
            }
            break;
            
        case '<': 
        {
            ++Tzer->At;
            switch (*Tzer->At) {
                case '<': 
                    ++Tzer->At;
                    switch(*Tzer->At) {
                        case '=': Token.Type = LBitShiftEquals; ++Tzer->At; return Token;
                        default: Token.Type = LBitShift; ++Tzer->At; return Token;
                    }
                case '=': Token.Type = LessEquals; ++Tzer->At; return Token;
                default:  Token.Type = Less; return Token;
            }
        } break;

        case '>': 
        {
            ++Tzer->At;
            switch (*Tzer->At) {
                case '<': 
                    ++Tzer->At;
                    switch(*Tzer->At) {
                        case '=': Token.Type = RBitShiftEquals; ++Tzer->At; return Token;
                        default: Token.Type = RBitShift; ++Tzer->At; return Token;
                    }
                case '=': Token.Type = GreaterEquals; ++Tzer->At; return Token;
                default:  Token.Type = Greater; return Token;
            }
        } break;

        case '\'':
        {
            ++Tzer->At;
            // TODO this is wrong
            if (*(Tzer->At + 1) && *(Tzer->At + 2) == '\'') {
                ++Tzer->At;
                Token.Type = CharacterConstant;
                Token.Text = Tzer->At;
                Tzer->At++;
                return Token;
            }
            ReportScanError(Tzer, "Malformed character literal");
        } break;

        case '"':
        {
            Token.Type = StringLiteral;
            const char* C = Token.Text = ++Tzer->At;
            while (*C != '\0') {
                if (*C == '\\') {
                    switch(*(Tzer->At + 1)) {
                        case 'a':
                        case 'b':
                        case 'e':
                        case 'f':
                        case 'n':
                        case 'r':
                        case 't':
                        case 'v':
                        case '"':
                        case '\'':
                        case '?':
                        case '\\':
                        {
                            C += 2;
                            continue;
                        } break;

                        case 'x':
                        case 'X':
                        {
                            while (('0' <= *C && *C <= '9') ||
                                   ('a' <= *C && *C <= 'f') ||
                                   ('A' <= *C && *C <= 'F'))
                            {
                                ++C;
                            }
                            continue;
                        } break;
                        
                        default:
                        {
                            if ('0' <= *C && *C <= '7') {
                                const char* Dig = C;
                                while ('0' <= *Dig && *Dig <= '7' && (Dig - C) < 3) ++Dig;
                                C = Dig;
                                continue;
                            }

                            // No known character escape sequence. Just eat a
                            // character, we'll emit an error later when we try
                            // to convert the string to an actual byte array.
                            if (*C != '\0') ++C;
                            continue;
                        } break;
                    }
                }
                // If we got here, there was no escape sequence including the
                // current character. We can trust that if it's a quote mark, it
                // actually ends the string.
                if (*C == '"') {
                    Token.TextLength = C - Tzer->At;
                    Tzer->At = C + 1;
                    return Token;
                }

                if (*C != '\0') ++C;
            }
            // Hit EOF in the middle of string, report error
            ReportScanError(Tzer, "EOF in the middle of string literal");
        } break;

        default:
        {
            // We can get here from above if the literal starts with .
int_scan:
float_scan:
            if (('0' <= *Tzer->At && *Tzer->At <= '9') || *Tzer->At == '.') 
            {
                Token.Text = Tzer->At;

                if (*Tzer->At == '.') {
                    Token.Type = FloatingConstant;
                    goto float_scan_significand;
                } 
                else 
                {
                    if (*Tzer->At == '0') {
                        Tzer->At++;
                        Token.Type = IntegerConstant;
                        if (*Tzer->At == 'x') {
                            Tzer->At++;
                            goto int_scan_hex;   
                        }
                        else goto int_scan_octal;
                    }
                    
                    while ('0' <= *Tzer->At && *Tzer->At <= '9') *Tzer->At++;

                    switch(*Tzer->At) {
                        case '.': goto float_scan_significand;
                        case 'E':
                        case 'e': goto float_scan_exponent;

                        case 'F':
                        case 'f': goto float_scan_suffix;

                        default: {
                            Token.Type = IntegerConstant;
                            Token.TextLength = Tzer->At - Token.Text;
                            return Token;
                        } break;
                    }

float_scan_significand:
                    assert(*Tzer->At == '.');
                    Tzer->At++;
                    while (*Tzer->At && ('0' <= *Tzer->At && *Tzer->At <= '9')) {
                        ++Tzer->At;
                    }

                    switch(*Tzer->At) {
                        case 'E':
                        case 'e': goto float_scan_exponent;

                        case 'F':
                        case 'f': goto float_scan_suffix;

                        default: goto float_accept;
                    }

float_scan_exponent:
                    assert(*Tzer->At == 'e' || *Tzer->At == 'E');
                    Tzer->At++;
                    if (*Tzer->At == '+' || *Tzer->At == '-') {
                        Tzer->At++;
                    }
                    while (*Tzer->At && ('0' <= *Tzer->At && *Tzer->At <= '9')) {
                        Tzer->At++;
                    }

float_scan_suffix:
                    switch (*Tzer->At) {
                        case 'f':
                        case 'F': Tzer->At++; goto float_accept;
                        default: goto float_accept;
                    }
float_accept:
                    Token.Type = FloatingConstant;
                    Token.TextLength = Tzer->At - Token.Text;
                    return Token;

int_scan_hex:
                    while(*Tzer->At && 
                          (('0' <= *Tzer->At && *Tzer->At <= '9') ||
                          ('a' <= *Tzer->At && *Tzer->At <= 'f') ||
                          ('A' <= *Tzer->At && *Tzer->At <= 'F'))) {
                        Tzer->At++;
                    }
                    goto int_accept;
int_scan_octal:
                    while(*Tzer->At && 
                          ('0' <= *Tzer->At && *Tzer->At <= '9')) {
                        Tzer->At++;
                    }
                    goto int_accept;
int_suffix:

int_accept:
                    Token.Type = IntegerConstant;
                    Token.TextLength = Tzer->At - Token.Text;
                    return Token;
                }
            }

            if (IsWordCharacter(*Tzer->At)) {
                Token.Type = Identifier;
                Token.Text = Tzer->At++;
                while (IsWordCharacter(*Tzer->At) || IsDigit(*Tzer->At)) Tzer->At++;
                Token.TextLength = Tzer->At - Token.Text;

                // TODO perfect hashing
                Token.Type = GetKeywordType(Token);

                return Token;
            }

            ReportScanError(Tzer, "Unrecognized character %c", *Tzer->At);
        } break;
    }

    if (*Tzer->At != '\0') {
        // Couldn't get anything. Try for another token.
        Tzer->At++;
        goto get_token;
    }

    Token.Type = EndOfFile;
    return Token;
}

token PeekToken(tokenizer* Tzer) {
    tokenizer Pre = *Tzer;
    token Result = GetToken(Tzer);
    *Tzer = Pre;
    return Result;
}
