#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "string"
using namespace std;


/*{ Sample program
  in TINY language
  compute factorial
}

read x; {input an integer}
if 0<x then {compute only if x>=1}
  fact:=1;
  repeat
    fact := fact * x;
    x:=x-1
  until x=0;
  write fact {output factorial}
end
*/

// sequence of statements separated by ;
// no procedures - no declarations
// all variables are integers
// variables are declared simply by assigning values to them :=
// variable names can include only alphabetic charachters(a:z or A:Z) and underscores
// if-statement: if (boolean) then [else] end
// repeat-statement: repeat until (boolean)
// boolean only in if and repeat conditions < = and two mathematical expressions
// math expressions integers only, + - * / ^
// I/O read write
// Comments {}

////////////////////////////////////////////////////////////////////////////////////
// Strings /////////////////////////////////////////////////////////////////////////

bool Equals(const char* a, const char* b)
{
    return strcmp(a, b)==0;
}

bool StartsWith(const char* a, const char* b)
{
    int nb=strlen(b);
    return strncmp(a, b, nb)==0;
}

void Copy(char* a, const char* b, int n=0)
{
    if(n>0) {strncpy(a, b, n); a[n]=0;}
    else strcpy(a, b);
}

void AllocateAndCopy(char** a, const char* b)
{
    if(b==0) {*a=0; return;}
    int n=strlen(b);
    *a=new char[n+1];
    strcpy(*a, b);
}

////////////////////////////////////////////////////////////////////////////////////
// Input and Output ////////////////////////////////////////////////////////////////

#define MAX_LINE_LENGTH 10000

struct InFile
{
    FILE* file;
    int cur_line_num;

    char line_buf[MAX_LINE_LENGTH];
    int cur_ind, cur_line_size;

    InFile(const char* str) {file=0; if(str) file=fopen(str, "r"); cur_line_size=0; cur_ind=0; cur_line_num=0;}
    ~InFile(){if(file) fclose(file);}

    void SkipSpaces()
    {
        while(cur_ind<cur_line_size)
        {
            char ch=line_buf[cur_ind];
            if(ch!=' ' && ch!='\t' && ch!='\r' && ch!='\n') break;
            cur_ind++;
        }
    }

    bool SkipUpto(const char* str)
    {
        while(true)
        {
            SkipSpaces();
            while(cur_ind>=cur_line_size) {if(!GetNewLine()) return false; SkipSpaces();}

            if(StartsWith(&line_buf[cur_ind], str))
            {
                cur_ind+=strlen(str);
                return true;
            }
            cur_ind++;
        }
        return false;
    }

    bool GetNewLine()
    {
        cur_ind=0; line_buf[0]=0;
        if(!fgets(line_buf, MAX_LINE_LENGTH, file)) return false;
        cur_line_size=strlen(line_buf);
        if(cur_line_size==0) return false; // End of file
        cur_line_num++;
        return true;
    }

    char* GetNextTokenStr()
    {
        SkipSpaces();
        while(cur_ind>=cur_line_size) {if(!GetNewLine()) return 0; SkipSpaces();}
        return &line_buf[cur_ind];
    }

    void Advance(int num)
    {
        cur_ind+=num;
    }
};

struct OutFile
{
    FILE* file;
    OutFile(const char* str) {file=0; if(str) file=fopen(str, "w");}
    ~OutFile(){if(file) fclose(file);}

    void Out(const char* s)
    {
        fprintf(file, "%s\n", s); fflush(file);
    }
};

////////////////////////////////////////////////////////////////////////////////////
// Compiler Parameters /////////////////////////////////////////////////////////////

struct CompilerInfo
{
    InFile in_file;
    OutFile out_file;
    OutFile debug_file;

    CompilerInfo(const char* in_str, const char* out_str, const char* debug_str)
                : in_file(in_str), out_file(out_str), debug_file(debug_str)
    {
    }
};

////////////////////////////////////////////////////////////////////////////////////
// Scanner /////////////////////////////////////////////////////////////////////////

#define MAX_TOKEN_LEN 40

enum TokenType{
                IF, THEN, ELSE, END, REPEAT, UNTIL, READ, WRITE,
                ASSIGN, EQUAL, LESS_THAN,
                PLUS, MINUS, TIMES, DIVIDE, POWER,
                SEMI_COLON,
                LEFT_PAREN, RIGHT_PAREN,
                LEFT_BRACE, RIGHT_BRACE,
                ID, NUM,
                ENDFILE, ERROR
              };

// Used for debugging only /////////////////////////////////////////////////////////
const char* TokenTypeStr[]=
            {
                "If", "Then", "Else", "End", "Repeat", "Until", "Read", "Write",
                "Assign", "Equal", "LessThan",
                "Plus", "Minus", "Times", "Divide", "Power",
                "SemiColon",
                "LeftParen", "RightParen",
                "LeftBrace", "RightBrace",
                "ID", "Num",
                "EndFile", "Error"
            };

struct Token
{
    TokenType type;
    char str[MAX_TOKEN_LEN+1];

    Token(){str[0]=0; type=ERROR;}
    Token(TokenType _type, const char* _str) {type=_type; Copy(str, _str);}
};

const Token reserved_words[]=
{
    Token(IF, "if"),
    Token(THEN, "then"),
    Token(ELSE, "else"),
    Token(END, "end"),
    Token(REPEAT, "repeat"),
    Token(UNTIL, "until"),
    Token(READ, "read"),
    Token(WRITE, "write")
};
const int num_reserved_words=sizeof(reserved_words)/sizeof(reserved_words[0]);

// if there is tokens like < <=, sort them such that sub-tokens come last: <= <
// the closing comment should come immediately after opening comment
const Token symbolic_tokens[]=
{
    Token(ASSIGN, ":="),
    Token(EQUAL, "="),
    Token(LESS_THAN, "<"),
    Token(PLUS, "+"),
    Token(MINUS, "-"),
    Token(TIMES, "*"),
    Token(DIVIDE, "/"),
    Token(POWER, "^"),
    Token(SEMI_COLON, ";"),
    Token(LEFT_PAREN, "("),
    Token(RIGHT_PAREN, ")"),
    Token(LEFT_BRACE, "{"),
    Token(RIGHT_BRACE, "}")
};
const int num_symbolic_tokens=sizeof(symbolic_tokens)/sizeof(symbolic_tokens[0]);

inline bool IsDigit(char ch){return (ch>='0' && ch<='9');}
inline bool IsLetter(char ch){return ((ch>='a' && ch<='z') || (ch>='A' && ch<='Z'));}
inline bool IsLetterOrUnderscore(char ch){return (IsLetter(ch) || ch=='_');}
inline int countID(const char* str)
{
    int i=0;
    while(IsLetterOrUnderscore(str[i])) i++;
    return i;
}
inline int countNum(const char* str)
{
    int i=0;
    while(IsDigit(str[i])) i++;
    return i;
}
// returns true if the token is a reserved word
bool IsReservedWord(const char* str, Token& token)
{

}
// returns true if the token is a symbolic token
bool IsSymbolicToken(const char* str, Token& token)
{
    for(int i=0; i<num_symbolic_tokens; i++)
    {
        if(Equals(str, symbolic_tokens[i].str))
        {
            token=symbolic_tokens[i];
            return true;
        }
    }
    return false;
}
// returns true if the token is a number
bool IsNumber(const char* str, Token& token)
{
    if(!IsDigit(str[0])) return false;

    int i=0;
    while(IsDigit(str[i])) i++;
    if(str[i]=='.')
    {
        i++;
        while(IsDigit(str[i])) i++;
    }
    if(str[i]!='\0') return false;

    token.type=NUM;
    Copy(token.str, str);
    return true;
}
// returns true if the token is an identifier
bool IsIdentifier(const char* str, Token& token)
{
    if(!IsLetterOrUnderscore(str[0])) return false;

    int i=0;
    while(IsLetterOrUnderscore(str[i]) || IsDigit(str[i])) i++;
    if(str[i]!='\0') return false;

    token.type=ID;
    Copy(token.str, str);
    return true;
}
// returns true if the token is a comment
bool IsComment(const char* str, Token& token)
{
    if(str[0]!='{') return false;

    int i=1;
    while(str[i]!='}') i++;
    if(str[i+1]!='\0') return false;

    token.type=ERROR;
    return true;
}
// returns true if the token is a string
bool IsString(const char* str, Token& token)
{
    if(str[0]!='"') return false;

    int i=1;
    while(str[i]!='"') i++;
    if(str[i+1]!='\0') return false;

    token.type=ERROR;
    return true;
}
// returns true if the token is a valid token
bool IsValidToken(const char* str, Token& token)
{

    if(IsReservedWord(str, token)) return true;
    if(IsSymbolicToken(str, token)) return true;
    if(IsNumber(str, token)) return true;
    if(IsIdentifier(str, token)) return true;
    if(IsComment(str, token)) return true;


    token.type=ERROR;
    return false;
}

struct Parser{
    CompilerInfo* compInfo;
    Token past_token;

    Parser(CompilerInfo* _comp) {compInfo=_comp;}

    void PrintToken(Token token){
        //"[%d]\t%s\t(%s) \n",compInfo->in_file.cur_line_num, token.str, TokenTypeStr[token.type]
        string str ="[" + to_string(compInfo->in_file.cur_line_num) + "]\t" + token.str + "\t(" + TokenTypeStr[token.type] + ")";
        compInfo->out_file.Out(str.c_str());
    }
    void GetNextToken(){
        if(past_token.type==ENDFILE) return;
        if(past_token.type==LEFT_BRACE)
        {
            compInfo->in_file.SkipUpto("}");
            past_token.type=RIGHT_BRACE;
            past_token.str[0]='}';
            return;
        }

        char* current_token_str=compInfo->in_file.GetNextTokenStr();
        if(current_token_str==nullptr)
        {
            past_token.type=ENDFILE;
            return;
        }

        if(IsLetterOrUnderscore(current_token_str[0]))
        {
//               if(IsReservedWord(current_token_str, past_token)) return;
//                if(IsIdentifier(current_token_str, past_token)) return;
            for(int i=0; i<num_reserved_words; i++)
            {
                if(StartsWith(current_token_str, reserved_words[i].str))
                {
                    past_token=reserved_words[i];
                    compInfo->in_file.Advance(strlen(reserved_words[i].str));
                    return;
                }
            }
            past_token.type=ID;
            Copy(past_token.str, current_token_str, countID(current_token_str));
            compInfo->in_file.Advance(strlen(past_token.str));
            return;
        }
        if(IsDigit(current_token_str[0]))
        {
            past_token.type=NUM;
            Copy(past_token.str, current_token_str, countNum(current_token_str));
            compInfo->in_file.Advance(strlen(past_token.str));
            return;
        }
        for(int i=0; i < num_symbolic_tokens; ++i)
        {
            if(StartsWith(current_token_str, symbolic_tokens[i].str))
            {
                past_token=symbolic_tokens[i];
                compInfo->in_file.Advance(strlen(symbolic_tokens[i].str));
                return;
            }
        }



        past_token.type=ERROR;
        Copy(past_token.str, current_token_str, countID(current_token_str));
        compInfo->in_file.Advance(strlen(past_token.str));
        return;

    }
};

int main(){

//    Token token;
//    char str[MAX_TOKEN_LEN+1];
//    while(scanf("%s", str)!=EOF)
//    {
//        if(IsValidToken(str, token))
//        {
//            printf("[%s]\t[%s]\t[%s],\n", str, token.str, TokenTypeStr[token.type]);
//        }else{
//            printf("[%s]\t[%s]\t[%s],\n", str, token.str, TokenTypeStr[token.type]);
//        }
//    }
    CompilerInfo compInfo("input.txt", "output.txt", "error.txt");
    Parser parser(&compInfo);
    parser.GetNextToken();
    while(parser.past_token.type!=ENDFILE)
    {
        parser.PrintToken(parser.past_token);
        parser.GetNextToken();
    }
    return 0;
}
