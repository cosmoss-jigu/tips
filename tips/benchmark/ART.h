//
// Created by florian on 18.11.15.
//

#ifndef ART_ROWEX_TREE_H
#define ART_ROWEX_TREE_H
#include "ROWEX/N.h"

using namespace ART;

namespace ART_ROWEX {

    class Tree {
    public:
        using LoadKeyFunction = void (*)(TID tid, Key &key);
        void *operator new(size_t size) {
            void *ret;
            ret = tips_alloc(size);
            return ret;
    	}
        void operator delete(void *p) {
            nvm_free(p);
    	}
    private:
        N *const root;

        void *checkKey(const Key *ret, const Key *k) const;

        LoadKeyFunction loadKey;

        Epoche epoche{256};

    public:
        enum class CheckPrefixResult : uint8_t {
            Match,
            NoMatch,
            OptimisticMatch
        };

        enum class CheckPrefixPessimisticResult : uint8_t {
            Match,
            NoMatch,
            SkippedLevel
        };

        enum class PCCompareResults : uint8_t {
            Smaller,
            Equal,
            Bigger,
            SkippedLevel
        };
        enum class PCEqualsResults : uint8_t {
            BothMatch,
            Contained,
            NoMatch,
            SkippedLevel
        };
        static CheckPrefixResult checkPrefix(N* n, const Key *k, uint32_t &level);

        static CheckPrefixPessimisticResult checkPrefixPessimistic(N *n, const Key *k, uint32_t &level,
                                                                   uint8_t &nonMatchingKey,
                                                                   Prefix &nonMatchingPrefix,
                                                                   LoadKeyFunction loadKey);

        static PCCompareResults checkPrefixCompare(const N* n, const Key *k, uint32_t &level, LoadKeyFunction loadKey);

        static PCEqualsResults checkPrefixEquals(const N* n, uint32_t &level, const Key *start, const Key *end, LoadKeyFunction loadKey);

    public:

        Tree(LoadKeyFunction loadKey);

        Tree(const Tree &) = delete;

        Tree(Tree &&t) : root(t.root), loadKey(t.loadKey) { }

        ~Tree();

        ThreadInfo getThreadInfo();

        void *art_lookup(void *p_head, const Key *k, ThreadInfo &threadEpocheInfo) const;

        bool lookupRange(const Key *start, const Key *end, const Key *continueKey, Key *result[], std::size_t resultLen,
                         std::size_t &resultCount, ThreadInfo &threadEpocheInfo) const;

        void art_insert(void *p_head, const Key *k, ThreadInfo &epocheInfo);

        void art_remove(void *p_head, const Key *k, ThreadInfo &epocheInfo);
    };
}
#endif //ART_ROWEX_TREE_H
