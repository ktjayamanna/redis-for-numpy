This module implements Vector Sets for Redis, a new Redis data type similar
to Sorted Sets but having string elements associated to a vector instead of
a score. The fundamental goal of Vector Sets is to make possible adding items,
and later get a subset of the added items that are the most similar to a
specified vector (often a learned embedding), or the most similar to the vector
of an element that is already part of the Vector Set.

Moreover, Vector sets implement optional hybrid search capabilities: it is possible to associate attributes to all or to a subset of elements in the set, and then, using the `FILTER` option of the `VSIM` command, to ask for items similar to a given vector but also passing a filter specified as a simple mathematical expression (Like `".year > 1950"` or similar).

## Installation

Build with:

    make

Then load the module with the following command line, or by inserting the needed directives in the `redis.conf` file.

    ./redis-server --loadmodule vset.so

To run tests, I suggest using this:

    ./redis-server --save "" --enable-debug-command yes

The execute the tests with:

    ./test.py

## Reference of available commands

**VADD: add items into a vector set**

    VADD key [REDUCE dim] FP32|VALUES vector element [CAS] [NOQUANT] [BIN] [Q8]
             [EF build-exploration-factor] [SETATTR <attributes>]

Add a new element into the vector set specified by the key.
The vector can be provided as FP32 blob of values, or as floating point
numbers as strings, prefixed by the number of elements (3 in the example):

    VADD mykey VALUES 3 0.1 1.2 0.5 my-element

Meaning of the options:

`REDUCE` implements random projection, in order to reduce the
dimensionality of the vector. The projection matrix is saved and reloaded
along with the vector set.

`CAS` performs the operation partially using threads, in a
check-and-set style. The neighbor candidates collection, which is slow, is
performed in the background, while the command is executed in the main thread.

`NOQUANT` forces the vector to be created (in the first VADD call to a given key) without integer 8 quantization, which is otherwise the default.

`BIN` forces the vector to use binary quantization instead of int8. This is much faster and uses less memory, but has impacts on the recall quality.

`Q8` forces the vector to use signed 8 bit quantization. This is the default, and the option only exists in order to make sure to check at insertion time if the vector set is of the same format.

`EF` plays a role in the effort made to find good candidates when connecting the new node to the existing HNSW graph. The default is 200. Using a larger value, may help to have a better recall. To improve the recall it is also possible to increase `EF` during `VSIM` searches.

`SETATTR` associates attributes to the newly created entry or update the entry attributes (if it already exists). It is the same as calling the `VSETATTR` attribute separately, so please check the documentation of that command in the hybrid search section of this documentation.

**VSIM: return elements by vector similarity**

    VSIM key [ELE|FP32|VALUES] <vector or element> [WITHSCORES] [COUNT num] [EF search-exploration-factor] [FILTER expression] [FILTER-EF max-filtering-effort]

The command returns similar vectors, for simplicity (and verbosity) in the following example, instead of providing a vector using FP32 or VALUES (like in `VADD`), we will ask for elements having a vector similar to a given element already in the sorted set:

    > VSIM word_embeddings ELE apple
     1) "apple"
     2) "apples"
     3) "pear"
     4) "fruit"
     5) "berry"
     6) "pears"
     7) "strawberry"
     8) "peach"
     9) "potato"
    10) "grape"

It is possible to specify a `COUNT` and also to get the similarity score (from 1 to 0, where 1 is identical, 0 is opposite vector) between the query and the returned items.

    > VSIM word_embeddings ELE apple WITHSCORES COUNT 3
    1) "apple"
    2) "0.9998867657923256"
    3) "apples"
    4) "0.8598527610301971"
    5) "pear"
    6) "0.8226882219314575"

The `EF` argument is the exploration factor: the higher it is, the slower the command becomes, but the better the index is explored to find nodes that are near to our query. Sensible values are from 50 to 1000.

For `FILTER` and `FILTER-EF` options, please check the hybrid search section of this documentation.

**VDIM: return the dimension of the vectors inside the vector set**

    VDIM keyname

Example:

    > VDIM word_embeddings
    (integer) 300

Note that in the case of vectors that were populated using the `REDUCE`
option, for random projection, the vector set will report the size of
the projected (reduced) dimension. Yet the user should perform all the
queries using full-size vectors.

**VCARD: return the number of elements in a vector set**

    VCARD key

Example:

    > VCARD word_embeddings
    (integer) 3000000


**VREM: remove elements from vector set**

    VREM key element

Example:

    > VADD vset VALUES 3 1 0 1 bar
    (integer) 1
    > VREM vset bar
    (integer) 1
    > VREM vset bar
    (integer) 0

VREM does not perform thumstone / logical deletion, but will actually reclaim
the memory from the vector set, so it is save to add and remove elements
in a vector set in the context of long running applications that continuously
update the same index.

**VEMB: return the approximated vector of an element**

    VEMB key element

Example:

    > VEMB word_embeddings SQL
      1) "0.18208661675453186"
      2) "0.08535309880971909"
      3) "0.1365649551153183"
      4) "-0.16501599550247192"
      5) "0.14225517213344574"
      ... 295 more elements ...

Because vector sets perform insertion time normalization and optional
quantization, the returned vector could be approximated. `VEMB` will take
care to de-quantized and de-normalize the vector before returning it.

It is possible to ask VEMB to return raw data, that is, the interal representation used by the vector: fp32, int8, or a bitmap for binary quantization. This behavior is triggered by the `RAW` option of of VEMB:

    VEMB word_embedding apple RAW

In this case the return value of the command is an array of three or more elements:
1. The name of the quantization used, that is one of: "fp32", "bin", "q8".
2. The a string blob containing the raw data, 4 bytes fp32 floats for fp32, a bitmap for binary quants, or int8 bytes array for q8 quants.
3. A float representing the l2 of the vector before normalization. You need to multiply by this vector if you want to de-normalize the value for any reason.

For q8 quantization, an additional elements is also returned: the quantization
range, so the integers from -127 to 127 represent (normalized) components
in the range `-range`, `+range`.

**VLINKS: introspection command that shows neighbors for a node**

    VLINKS key element [WITHSCORES]

The command reports the neighbors for each level.

**VINFO: introspection command that shows info about a vector set**

    VINFO key

Example:

    > VINFO word_embeddings
     1) quant-type
     2) int8
     3) vector-dim
     4) (integer) 300
     5) size
     6) (integer) 3000000
     7) max-level
     8) (integer) 12
     9) vset-uid
    10) (integer) 1
    11) hnsw-max-node-uid
    12) (integer) 3000000

**VSETATTR: associate or remove the JSON attributes of elements**

    VSETATTR key element "{... json ...}"

Each element of a vector set can be optionally associated with a JSON string
in order to use the `FILTER` option of `VSIM` to filter elements by scalars
(see the hybrid search section for more information). This command can set,
update (if already set) or delete (if you set to an empty string) the
associated JSON attributes of an element.

The command returns 0 if the element or the key don't exist, without
raising an error, otherwise 1 is returned, and the element attributes
are set or updated.

**VGETATTR: retrieve the JSON attributes of elements**

    VGET key element

The command returns the JSON attribute associated with an element, or
null if there is no element associated, or no element at all, or no key.

## Hybrid search capabilities

Each element of the vector set can be associated with a set of attributes specified as a JSON blob:

    > VADD vset VALUES 3 1 1 1 a SETATTR '{"year": 1950}'
    (integer) 1
    > VADD vset VALUES 3 -1 -1 -1 b SETATTR '{"year": 1951}'
    (integer) 1

Specifying an attribute with the `SETATTR` option of `VADD` is exactly equivalent to adding an element and then setting (or updating, if already set) the attributes JSON string. Also the symmetrical `VGETATTR` command returns the attribute associated to a given element.

    > VAD vset VALUES 3 0 1 0 c
    (integer) 1
    > VSETATTR vset c '{"year": 1952}'
    (integer) 1
    > VGETATTR vset c
    "{\"year\": 1952}"

At this point, I may use the FILTER option of VSIM to only ask for the subset of elements that are verified by my expression:

    > VSIM vset VALUES 3 0 0 0 FILTER '.year > 1950'
    1) "c"
    2) "b"

The items will be returned again in order of similarity (most similar first), but only the items with the year field matching the expression is returned.

The expressions are similar to what you would write inside the `if` statement of JavaScript or other familiar programming languages: you can use `and`, `or`, the obvious math operators like `+`, `-`, `/`, `>=`, `<`, ... and so forth (see the expressions section for more info). The selectors of the JSON object attributes start with a dot followed by the name of the key inside the JSON objects.

Elements with invalid JSON or not having a given specified field **are considered as not matching** the expression, but will not generate any error at runtime.

## FILTER expressions capabilities

Fill me.

## FILTER effort

Fill me.

## Known bugs

* When VADD with REDUCE is replicated, we should probably send the replicas the random matrix, in order for VEMB to read the same things. This is not critical, because the behavior of VADD / VSIM should be transparent if you don't look at the transformed vectors, but still probably worth doing.
* Replication code is pretty much untested, and very vanilla (replicating the commands verbatim).

## Implementation details

Vector sets are based on the `hnsw.c` implementation of the HNSW data structure with extensions for speed and functionality.

The main features are:

* Proper nodes deletion with relinking.
* 8 bits quantization.
* Threaded queries.
