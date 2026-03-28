#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>

using namespace std;

const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
int alphabet_inv[256];
const int A_SIZE = sizeof(alphabet)/sizeof(alphabet[0]);

struct Node;

vector<Node> trie;

struct Node
{
    int next[A_SIZE];
    bool end;
    Node()
    {
        for (int i = 0; i < A_SIZE; ++i)
            next[i] = -1;
        end = false;
    }
    bool add(const string &str, int pos)
    {
        if (pos == str.length())
        {
            end = true;
            return true;
        }
        int p = alphabet_inv[(unsigned char)str[pos]];
        if (p < 0)
        {
            fprintf(stderr, "bad character %c\n", str[pos]);
            return false;
        }
        int np = next[p];
        if (np < 0)
        {
            np = trie.size();
            next[p] = np;
            trie.emplace_back();
        }
        return trie[np].add(str, pos + 1);
    }
};

bool rec_(const string &s, int pos, int node);

bool rec(const string &s, int pos, int node)
{
    //printf("%d %c %d = ?\n", pos, s[pos], node);
    bool res = rec_(s, pos, node);
    //printf("%d %c %d = %c\n", pos, s[pos], node, (res ? 't' : 'f'));
    //if (res)
    //    printf("%c %d\n", s[pos], node);
    return res;
}

bool rec_(const string &s, int pos, int node)
{
    if (pos == s.length())
        return (node == -3);
    char c = s[pos];
    pos += 1;
    if (c == '_' || c == '-')
    {
        if (node == -3) // delimeter expected
            return (rec(s, pos, -1) || rec(s, pos, 0) || rec(s, pos, -2));
        else
            return false;
    }
    if (c >= '0' && c <= '9')
    {
        if (node == -1)
            return (rec(s, pos, -3) || rec(s, pos, -1) || rec(s, pos, 0));
        if (node == -2) // digits after word
            return (rec(s, pos, -3) || rec(s, pos, -2));
        return false;
    }
    if (node == -1 || node == -2 || node == -3)
        return false;
    int p = alphabet_inv[(unsigned char)c];
    if (p < 0)
    {
        printf("bad char %c\n", c);
        return false;
    }
    const Node &n = trie[node];
    int ni = n.next[p];
    if (ni < 0)
        return false;
    if (rec(s, pos, ni))
        return true;
    if (trie[ni].end)
    {
        return (rec(s, pos, -2) || rec(s, pos, -3));
    }
    return false;
}

bool dict_check(const string s)
{
    return rec(s, 0, -1) || rec(s, 0, 0) || rec(s, 0, -2);
}

int main(int argc, const char *argv[])
{
    if (argc != 2 && argc != 3 && argc != 4)
    {
        fprintf(stderr, "usage: ./filter_by_dict input.txt dictionary.txt [min_weight]\n");
        return -1;
    }
    int min_weight;
    if (argc > 2)
    { 
        if (sscanf(argv[3], "%d", &min_weight) != 1)
        {
            printf("min_weight not a number\n");
            return -2;
        }
    }
    for (int i = 0; i < 256; ++i)
        alphabet_inv[i] = -1;
    for (int i = 0; i < A_SIZE; ++i)
        alphabet_inv[(unsigned char)alphabet[i]] = i;
    FILE *f = fopen(argv[2], "r");
    if (!f)
    {
        printf("Can't open %s\n", argv[2]);
        return -2;
    }
    trie.emplace_back();
    char buff[10000];
    while (true)
    {
        if (!fgets(buff, sizeof(buff)/sizeof(buff[0]), f))
            break;
        for (int i = 0; buff[i]; ++i)
            if (buff[i] == '\r' || buff[i] == '\n')
            {
                buff[i] = 0;
                break;
            }
        if (!buff[0])
            continue;
        trie[0].add(buff, 0);
    }
    fclose(f);
    f = fopen(argv[1], "r");
    if (!f)
    {
        printf("Can't open %s\n", argv[1]);
        return -2;
    }
    while (true)
    {
        if (!fgets(buff, sizeof(buff) / sizeof(buff[0]), f))
            break;
        for (int i = 0; buff[i]; ++i)
            if (buff[i] == '\r' || buff[i] == '\n')
            {
                buff[i] = 0;
                break;
            }
        if (!buff[0])
            continue;
        string s(buff);
        if (dict_check(s))
        {
            int prev = -1;
            int weight = 0;
            for (int i = 0; buff[i]; ++i)
            {
                if (buff[i] >= 'a' && buff[i] <= 'z')
                {
                    if (prev < 0)
                        prev = i;
                }
                else
                {
                    if (prev >= 0)
                        weight += (i - prev) * (i - prev);
                    prev = -1;
                }
            }
            if (prev >= 0)
                weight += (s.length() - prev) * (s.length() - prev);
            if (weight >= min_weight)
                printf("%s=%d\n", buff, weight);
        }
    }
    fclose(f);
    return 0;
}

