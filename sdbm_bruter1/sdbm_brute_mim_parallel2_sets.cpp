#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_set>
#include <memory>
#include <map>
#include <set>
#include <functional>
#include <bitset>
#include <thread>
#include <mutex>
#include <atomic>

using namespace std;

const uint32_t BASE = 0x1003F;
const uint32_t BASE_INV = 2390421439;

static uint32_t fpow(uint32_t a, int n) {
    if (n <= 0)
        return 1;
    uint32_t r = fpow(a, n / 2);
    r *= r;
    if (n & 1)
        r *= a;
    return r;
}

static uint32_t sdbm(uint32_t hash, const string &str) {
    for (int i = 0; i < str.length(); i++) {
        hash = BASE * (hash + (unsigned char)str[i]);
    }
    return hash;
}

static void printvec(const vector<int> &vec) {
    printf("[");
    for (auto v: vec)
        printf("%d, ", v);
    printf("]");
}

class Bruter {
public:
    struct Operator {
        uint32_t multiplier;
        uint32_t constant;
        Operator() : multiplier(1), constant(0) {}
        Operator(const string &s) {
            multiplier = 1;
            constant = 0;
            for (char c: s) {
                multiplier *= BASE;
                constant = (constant + (unsigned char)c) * BASE;
            }
        }
        uint32_t apply(uint32_t hash) const {
            return hash * multiplier + constant;
        }
    };
    struct Word {
        string word;
        Operator op;
        Word() {}
        Word(const string &s) : word(s), op(s) {}
    };
    vector<Word> dictionary;
    unordered_set<uint32_t> hashes;
    map<uint32_t, uint32_t> hashes_path;
    int maxdepth;
    string known_prefix;
    string known_suffix;
    Operator op_suffix;
    shared_ptr<bitset<(1LL<<32)>> mim_have;
    function<void (const Bruter &, uint32_t, uint32_t, const string&)> callback;
    Bruter() : maxdepth(0) { }
    Bruter(const vector<string> &dictionary, const vector<uint32_t> &hashes) : maxdepth(0) {
        for (auto h : hashes)
            this->hashes.insert(h);
        for (auto &s : dictionary)
            this->dictionary.emplace_back(s);
    }
    void mim_rec(uint32_t current, int depth) {
        mim_have->set(current, true);
        if (depth >= maxdepth)
            return;
        for (int i = 0; i < dictionary.size(); ++i) {
            mim_rec(dictionary[i].op.apply(current), depth + 1);
        }
    }
    void mim_rec2(uint32_t current, int depth, int length, vector<int> &words) const {
        uint32_t candidate = op_suffix.apply(current);
        for (auto want: hashes) {
            uint32_t looking = ((uint32_t)(want - candidate)) * fpow(BASE_INV, length);
            if ((*mim_have)[looking]) {
                string s;
                for (int i : words)
                    s += dictionary[i].word;
                s += known_suffix;
                callback(*this, want, looking, s);
            }
        }
        if (depth >= maxdepth)
            return;
        for (int i = 0; i < dictionary.size(); ++i) {
            const Operator &op = dictionary[i].op;
            words.push_back(i);
            mim_rec2(op.apply(current), depth + 1, length + dictionary[i].word.length(), words);
            words.pop_back();
        }
    }
    void brute_mim_internal(int depth) {
        op_suffix = Operator(known_suffix);
        mim_have = make_shared<bitset<(1LL<<32)>>();
        maxdepth = depth / 2;
        uint32_t known_prefix_sdbm = sdbm(0, known_prefix);
        mim_rec(known_prefix_sdbm, 0);
        maxdepth = depth - (depth / 2);
        vector<int> words;
        mim_rec2(0, 0, known_suffix.length(), words);
    }
    void brute_mim(int depth) {
        if (depth <= 1) {
            brute(depth); // fallback into simple brute
            return;
        }
        Bruter phase1 = *this, phase2 = *this;
        map<uint32_t, vector<pair<uint32_t,string>>> looking_hash; 
        phase2.hashes.clear();
        phase1.callback = [&phase2, &looking_hash] (const Bruter &B, uint32_t want, uint32_t looking, const string &s) {
            printf("%08x = sdbm(%08x + \"%s\")\n", want, looking, s.c_str());
            phase2.hashes.insert(looking);
            looking_hash[looking].push_back({want, s});
        };
        phase1.brute_mim_internal(depth);
        phase2.known_suffix = "";
        phase2.callback = [this, &looking_hash] (const Bruter &B, uint32_t want, uint32_t looking, const string &s) {
            printf("%08x = sdbm(0, \"%s\")\n", want, s.c_str());
            if (sdbm(0, s) != want)
                printf("error1: %u != %u\n", sdbm(0, s), want);
            const vector<pair<uint32_t, string>> &looking_vec = looking_hash[want];
            for (auto &p : looking_vec)
            {
                uint32_t whole_hash = p.first;
                string whole = s + p.second;
                printf("! %08x = sdbm(0, \"%s\")\n", whole_hash, whole.c_str());
                if (sdbm(0, whole) != whole_hash)
                    printf("error2: %u != %u\n", sdbm(0, whole), whole_hash);
                this->callback(*this, whole_hash, 0, whole);
            }
        };
        phase2.brute(depth - (int)(depth / 2));
    }
    void rec(uint32_t current, int depth, vector<int> &words) const
    {
        uint32_t candidate = op_suffix.apply(current);
        if (hashes.find(candidate) != hashes.end())
        {
            string s = known_prefix;
            for (int i : words)
                s += dictionary[i].word;
            s += known_suffix;
            callback(*this, candidate, 0, s);
        }
        if (depth >= maxdepth)
            return;
        for (int i = 0; i < dictionary.size(); ++i)
        {
            const Operator &op = dictionary[i].op;
            words.push_back(i);
            rec(op.apply(current), depth + 1, words);
            words.pop_back();
        }
    }
    void brute(int depth)
    {
        maxdepth = depth;
        vector<int> words;
        op_suffix = Operator(known_suffix);
        rec(sdbm(0, known_prefix), 0, words);
    }
    struct ParallelContext {
        vector<int> start; // inclusive
        vector<int> end; // not inclusive
        vector<int> words; // size = depth + 1
        typedef vector<atomic<uint8_t>> bits_type;
        bits_type *bits_data;
        Bruter *b;
        void bit_set(uint32_t pos) {
            (*bits_data)[pos / 8] |= (1 << (pos & 7));
        }
        bool bit_get(uint32_t pos) {
            return ((*bits_data)[pos / 8] >> (pos & 7)) & 1;
        }
        void rec(uint32_t current, int depth, bool larger_than_start, bool less_than_end) {
            uint32_t candidate = b->op_suffix.apply(current);
            if (b->hashes.find(candidate) != b->hashes.end()) {
                string s = b->known_prefix;
                for (int i : words)
                    s += b->dictionary[i].word;
                s += b->known_suffix;
                b->callback(*b, candidate, 0, s);
            }
            if (depth <= 0)
                return;
            const int lim = (less_than_end ? b->dictionary.size() : end[depth - 1]);
            for (int i = (larger_than_start ? 0 : start[depth - 1]); i < lim; ++i) {
                const Operator &op = b->dictionary[i].op;
                words.push_back(i);
                rec(op.apply(current), depth - 1, larger_than_start, true);
                words.pop_back();
                larger_than_start = true;
            }
            if (!less_than_end) {
                const Operator &op = b->dictionary[lim].op;
                words.push_back(lim);
                rec(op.apply(current), depth - 1, larger_than_start, false);
                words.pop_back();
            }
        }
        void run(uint32_t current, int depth) {
            rec(current, depth, false, 0 < end[depth]);
        }
        void mim_rec(uint32_t current, int depth, bool larger_than_start, bool less_than_end) {
            bit_set(current);
            if (depth <= 0)
                return;
            const int lim = (less_than_end ? b->dictionary.size() : end[depth - 1]);
            for (int i = (larger_than_start ? 0 : start[depth - 1]); i < lim; ++i) {
                mim_rec(b->dictionary[i].op.apply(current), depth - 1, larger_than_start, true);
                larger_than_start = true;
            }
            if (!less_than_end) {
                mim_rec(b->dictionary[lim].op.apply(current), depth - 1, larger_than_start, true);
            }
        }
        void mim_run(uint32_t current, int depth) {
            mim_rec(current, depth, false, 0 < end[depth]);
        }
        void mim_rec2(uint32_t current, int depth, int length, bool larger_than_start, bool less_than_end) {
            uint32_t candidate = b->op_suffix.apply(current);
            for (auto want: b->hashes) {
                uint32_t looking = ((uint32_t)(want - candidate)) * fpow(BASE_INV, length);
                if (bit_get(looking)) {
                    string s;
                    for (int i : words)
                        s += b->dictionary[i].word;
                    s += b->known_suffix;
                    b->callback(*b, want, looking, s);
                }
            }
            if (depth <= 0)
                return;
            const int lim = (less_than_end ? b->dictionary.size() : end[depth - 1]);
            for (int i = (larger_than_start ? 0 : start[depth - 1]); i < lim; ++i) {
                const Operator &op = b->dictionary[i].op;
                words.push_back(i);
                mim_rec2(op.apply(current), depth - 1, length + b->dictionary[i].word.length(), larger_than_start, true);
                words.pop_back();
                larger_than_start = true;
            }
            if (!less_than_end) {
                const Operator &op = b->dictionary[lim].op;
                words.push_back(lim);
                mim_rec2(op.apply(current), depth - 1, length + b->dictionary[lim].word.length(), larger_than_start, false);
                words.pop_back();
            }
        }
        void mim_run2(uint32_t current, int depth) {
            mim_rec2(current, depth, b->known_suffix.length(), false, 0 < end[depth]);
        }
    };
    vector<int> convert_to_vector(uint64_t index, int depth) {
        vector<int> res(depth + 1);
        for (int i = 0; i <= depth; ++i) {
            res[i] = index % dictionary.size();
            index /= dictionary.size();
        }
        return res;
    }
    void brute_parallel(int depth, int threads)
    {
        /*if (threads <= 1) {
            brute(depth); // fallback into simple one
            return;
        }*/
        op_suffix = Operator(known_suffix);
        uint64_t total = 1;
        for (int i = 0; i < depth; ++i) {
            if (total > (0xFFFFFFFFFFFFFFFFULL / dictionary.size())) {
                printf("depth is too large\n");
                return;
            }
            total *= dictionary.size();
        }
        uint64_t steps = (total % threads == 0 ? total / threads : total / threads + 1);
        vector<ParallelContext> contexts(threads);
        vector<thread> threads_vec;
        uint32_t known_prefix_sdbm = sdbm(0, known_prefix);
        for (int thread_idx = 0; thread_idx < threads; ++thread_idx) {
            ParallelContext &ctx = contexts[thread_idx];
            ctx.start = convert_to_vector(thread_idx * steps, depth);
            ctx.end = convert_to_vector((thread_idx + 1) * steps, depth);
            ctx.b = this;
            //printf("start = ");
            //printvec(ctx.start);
            //printf("\nend = ");
            //printvec(ctx.end);
            //printf("\n");
            //ctx.run(sdbm(0, known_prefix), depth);
            threads_vec.emplace_back(&ParallelContext::run, &ctx, known_prefix_sdbm, depth);
        }
        for (auto &t : threads_vec) {
            t.join();
        }
    }
    unique_ptr<ParallelContext::bits_type> brute_mim_parallel_internal_step1(int depth, int threads) {
        op_suffix = Operator(known_suffix);
        uint64_t total = 1;
        for (int i = 0; i < depth - (depth / 2); ++i) {
            if (total > (0xFFFFFFFFFFFFFFFFULL / dictionary.size())) {
                printf("depth is too large\n");
                return unique_ptr<ParallelContext::bits_type>();
            }
            total *= dictionary.size();
        }
        uint64_t steps = (total % threads == 0 ? total / threads : total / threads + 1);
        vector<ParallelContext> contexts(threads);
        vector<thread> threads_vec;
        unique_ptr<ParallelContext::bits_type> bits = make_unique<ParallelContext::bits_type>(1LL<<(32-3));
        for (int i = 0; i < bits->size(); ++i)
            (*bits)[i] = 0; 
        uint32_t known_prefix_sdbm = sdbm(0, known_prefix);
        for (int thread_idx = 0; thread_idx < threads; ++thread_idx) {
            ParallelContext &ctx = contexts[thread_idx];
            ctx.start = convert_to_vector(thread_idx * steps, depth);
            ctx.end = convert_to_vector((thread_idx + 1) * steps, depth);
            ctx.b = this;
            ctx.bits_data = &(*bits);
            //printf("start = ");
            //printvec(ctx.start);
            //printf("\nend = ");
            //printvec(ctx.end);
            //printf("\n");
            threads_vec.emplace_back(&ParallelContext::mim_run, &ctx, known_prefix_sdbm, depth - (depth / 2));
        }
        for (auto &t : threads_vec) {
            t.join();
        }
        return bits;
    }
    void brute_mim_parallel_internal_step2(const unique_ptr<ParallelContext::bits_type> &bits, int depth, int threads) {
        op_suffix = Operator(known_suffix);
        uint64_t total = 1;
        for (int i = 0; i < (depth / 2); ++i) {
            if (total > (0xFFFFFFFFFFFFFFFFULL / dictionary.size())) {
                printf("depth is too large\n");
                return;
            }
            total *= dictionary.size();
        }
        uint64_t steps = (total % threads == 0 ? total / threads : total / threads + 1);
        vector<ParallelContext> contexts(threads);
        vector<thread> threads_vec;
        for (int thread_idx = 0; thread_idx < threads; ++thread_idx) {
            ParallelContext &ctx = contexts[thread_idx];
            ctx.start = convert_to_vector(thread_idx * steps, depth);
            ctx.end = convert_to_vector((thread_idx + 1) * steps, depth);
            ctx.b = this;
            ctx.bits_data = &(*bits);
            ctx.words.clear();
            //printf("start = ");
            //printvec(ctx.start);
            //printf("\nend = ");
            //printvec(ctx.end);
            //printf("\n");
            threads_vec.emplace_back(&ParallelContext::mim_run2, &ctx, 0, depth / 2);
        }
        for (auto &t : threads_vec) {
            t.join();
        }
    }
    void brute_mim_parallel_internal(int depth, int threads) {
        fprintf(stderr, "step1\n");
        unique_ptr<ParallelContext::bits_type> bits = brute_mim_parallel_internal_step1(depth, threads);
        fprintf(stderr, "step2\n");
        if (bits)
            brute_mim_parallel_internal_step2(bits, depth, threads);
    }
    void brute_mim_parallel(int depth, int threads) {
        /*if (depth <= 1) {
            brute_parallel(depth); // fallback into simple brute
            return;
        }*/
        Bruter phase1 = *this, phase2 = *this;
        map<uint32_t, set<pair<uint32_t,string>>> looking_hash;
        mutex looking_mutex;
        phase2.hashes.clear();
        phase1.callback = [&phase2, &looking_hash, &looking_mutex] (const Bruter &B, uint32_t want, uint32_t looking, const string &s) {
            //printf("%08x = sdbm(%08x + \"%s\")\n", want, looking, s.c_str());
            lock_guard<mutex> guard(looking_mutex);
            phase2.hashes.insert(looking);
            looking_hash[looking].insert({want, s});
        };
        phase1.brute_mim_parallel_internal(depth, threads);
        phase2.known_suffix = "";
        fprintf(stderr, "final step\n");
        phase2.callback = [this, &looking_hash] (const Bruter &B, uint32_t want, uint32_t looking, const string &s) {
            //printf("%08x = sdbm(0, \"%s\")\n", want, s.c_str());
            //if (sdbm(0, s) != want)
            //    printf("error1: %u != %u\n", sdbm(0, s), want);
            const set<pair<uint32_t, string>> &looking_set = looking_hash[want];
            for (auto &p : looking_set) {
                uint32_t whole_hash = p.first;
                string whole = s + p.second;
                //printf("! %08x = sdbm(0, \"%s\")\n", whole_hash, whole.c_str());
                //if (sdbm(0, whole) != whole_hash)
                //    printf("error2: %u != %u\n", sdbm(0, whole), whole_hash);
                this->callback(*this, whole_hash, 0, whole);
            }
        };
        phase2.brute_parallel(depth - (depth / 2), threads);
    }
};

pair<uint32_t, uint32_t> get_path_and_fname_hashes(uint32_t path_fname_hash, uint32_t fname_ext_hash, const string &ext, int fname_len) {
    uint32_t ext_hash = sdbm(0, ext);
    uint32_t ext_len = ext.length();
    uint32_t fname_hash = (fname_ext_hash - ext_hash) * fpow(BASE_INV, ext_len);
    uint32_t path_hash = (path_fname_hash - fname_hash) * fpow(BASE_INV, fname_len);
    return {path_hash, fname_hash};
}

int main(int argc, const char *argv[]) {
    const int max_fname_len = 100;
    const int path_depth = 4;
    const int path_threads = 4;
    const string path_known_prefix = "textures/gui/";
    if (argc < 5 || argc > 8) {
        printf("Usage: sdbm_brute_mim dict_names dict_paths hashes depth [threads [known_prefix [known_suffix]]]\n");
        return -1;
    }
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        printf("Can't open %s\n", argv[1]);
        return -2;
    }
    char buff[10000];
    vector<string> dict_names;
    while (true) {
        if (!fgets(buff, sizeof(buff)/sizeof(buff[0]), f))
            break;
        for (int i = 0; buff[i]; ++i)
            if (buff[i] == '\r' || buff[i] == '\n') {
                buff[i] = 0;
                break;
            }
        if (!buff[0])
            continue;
        dict_names.emplace_back(buff);
    }
    fclose(f);

    f = fopen(argv[2], "r");
    if (!f) {
      printf("Can't open %s\n", argv[2]);
      return -2;
    }

    vector<string> dict_paths;
    while (true) {
      if (!fgets(buff, sizeof(buff) / sizeof(buff[0]), f))
        break;
      for (int i = 0; buff[i]; ++i)
        if (buff[i] == '\r' || buff[i] == '\n') {
          buff[i] = 0;
          break;
        }
      if (!buff[0])
        continue;
      dict_paths.emplace_back(buff);
    }
    fclose(f);

    f = fopen(argv[3], "r");
    if (!f) {
        printf("Can't open %s\n", argv[3]);
        return -3;
    }
    vector<uint32_t> hashes;
    map<uint32_t, uint32_t> hashes_path;
    while (true) {
        uint32_t hash, hash_path;
        if (fscanf(f, "%08x %08x", &hash, &hash_path) != 2)
            break;
        hashes.push_back(hash);

        if (hash_path) {
          hashes_path.insert({ hash, hash_path });
        }
    }
    fclose(f);
    if (hashes.size() != 1) {
        printf("single hash pair expected");
        return -10;
    }
    int depth = 0;
    if (sscanf(argv[4], "%d", &depth) != 1) {
        printf("%s is not a number", argv[3]);
        return -4;
    }
    int threads = 1;
    if (argc > 5)
    {
        if (sscanf(argv[5], "%d", &threads) != 1) {
            printf("%s is not a number", argv[5]);
            return -4;
        }
    }    
    string known_suffix, known_prefix; 
    if (argc > 6)
        known_prefix = argv[6];
    if (argc > 7)
        known_suffix = argv[7];
    Bruter b(dict_names, hashes);
    b.known_prefix = known_prefix;
    b.known_suffix = known_suffix;
    fprintf(stderr, "dict_names.size() = %d, dict_paths.size() = %d, hashes.size() = %d\n", (int)dict_names.size(), (int)dict_paths.size(), (int)hashes.size());
    fprintf(stderr, "known_prefix = \"%s\"\n", b.known_prefix.c_str());
    fprintf(stderr, "known_suffix = \"%s\"\n", b.known_suffix.c_str());
    b.callback = [] (const Bruter &B, uint32_t want, uint32_t looking, const string &s) {
        printf("%s\n", s.c_str());
    };
    b.brute_mim_parallel(depth, threads);
    return 0;
}

