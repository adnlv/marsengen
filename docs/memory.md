# Memory

## File structure

All multi-byte integers are little-endian.

### Header

First 4 bytes are a magic number `MRSN` (`4D52534E`). Next 2 bytes are a format
version.

```text
4D52534E,0001
```

### Vocabulary

Then goes the set of unique words. First 4 bytes tell how many unique words
there are. Each word is represented by its length (1 byte) followed by the set
of characters of the given length.

For example, 4 words: "n", "hi", "hello" and "transition" would look like this:

```text
00000004,01,6E,02,6869,05,68656C6C6F,0A,7472616E736974696F6E
                                     |  |
                                     |  The word ("transition")
                                     |
                                     Its length (10)
```

### Transitions

Then for each unique word (in the same order as the vocabulary) goes its
transition data. First 4 bytes are the number of distinct next-words. Next 4
bytes are the total observation count. Then for each transition: 4 bytes for the
destination word index (into the vocabulary) and 4 bytes for the count.

For example, if word 0 ("n") has 2 transitions — to word 2 ("hello") seen 3
times and to word 1 ("hi") seen 7 times:

```text
00000002,0000000A,00000002,00000003,00000001,00000007
```

A word with no transitions is just:

```text
00000000,00000000
```

## Merging

When loading memory and processing new text, transition counts are additive.
New words extend the vocabulary and get indices starting after the loaded ones.
