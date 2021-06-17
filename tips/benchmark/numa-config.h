enum {
    NUM_SOCKET = 8,
    NUM_PHYSICAL_CPU_PER_SOCKET = 28,
    SMT_LEVEL = 2,
};

const int OS_CPU_ID[NUM_SOCKET][NUM_PHYSICAL_CPU_PER_SOCKET][SMT_LEVEL] = {
    { /* socket id: 0 */
        { /* physical cpu id: 0 */
          0, 1,     },
        { /* physical cpu id: 1 */
          2, 3,     },
        { /* physical cpu id: 2 */
          4, 5,     },
        { /* physical cpu id: 3 */
          6, 7,     },
        { /* physical cpu id: 4 */
          8, 9,     },
        { /* physical cpu id: 5 */
          10, 11,     },
        { /* physical cpu id: 6 */
          12, 13,     },
        { /* physical cpu id: 8 */
          14, 15,     },
        { /* physical cpu id: 9 */
          16, 17,     },
        { /* physical cpu id: 10 */
          18, 19,     },
        { /* physical cpu id: 11 */
          20, 21,     },
        { /* physical cpu id: 12 */
          22, 23,     },
        { /* physical cpu id: 13 */
          24, 25,     },
        { /* physical cpu id: 14 */
          26, 27,     },
        { /* physical cpu id: 16 */
          28, 29,     },
        { /* physical cpu id: 17 */
          30, 31,     },
        { /* physical cpu id: 18 */
          32, 33,     },
        { /* physical cpu id: 19 */
          34, 35,     },
        { /* physical cpu id: 20 */
          36, 37,     },
        { /* physical cpu id: 21 */
          38, 39,     },
        { /* physical cpu id: 22 */
          40, 41,     },
        { /* physical cpu id: 24 */
          42, 43,     },
        { /* physical cpu id: 25 */
          44, 45,     },
        { /* physical cpu id: 26 */
          46, 47,     },
        { /* physical cpu id: 27 */
          48, 49,     },
        { /* physical cpu id: 28 */
          50, 51,     },
        { /* physical cpu id: 29 */
          52, 53,     },
        { /* physical cpu id: 30 */
          54, 55,     },
    },
    { /* socket id: 1 */
        { /* physical cpu id: 0 */
          56, 57,     },
        { /* physical cpu id: 1 */
          58, 59,     },
        { /* physical cpu id: 2 */
          60, 61,     },
        { /* physical cpu id: 3 */
          62, 63,     },
        { /* physical cpu id: 4 */
          64, 65,     },
        { /* physical cpu id: 5 */
          66, 67,     },
        { /* physical cpu id: 6 */
          68, 69,     },
        { /* physical cpu id: 8 */
          70, 71,     },
        { /* physical cpu id: 9 */
          72, 73,     },
        { /* physical cpu id: 10 */
          74, 75,     },
        { /* physical cpu id: 11 */
          76, 77,     },
        { /* physical cpu id: 12 */
          78, 79,     },
        { /* physical cpu id: 13 */
          80, 81,     },
        { /* physical cpu id: 14 */
          82, 83,     },
        { /* physical cpu id: 16 */
          84, 85,     },
        { /* physical cpu id: 17 */
          86, 87,     },
        { /* physical cpu id: 18 */
          88, 89,     },
        { /* physical cpu id: 19 */
          90, 91,     },
        { /* physical cpu id: 20 */
          92, 93,     },
        { /* physical cpu id: 21 */
          94, 95,     },
        { /* physical cpu id: 22 */
          96, 97,     },
        { /* physical cpu id: 24 */
          98, 99,     },
        { /* physical cpu id: 25 */
          100, 101,     },
        { /* physical cpu id: 26 */
          102, 103,     },
        { /* physical cpu id: 27 */
          104, 105,     },
        { /* physical cpu id: 28 */
          106, 107,     },
        { /* physical cpu id: 29 */
          108, 109,     },
        { /* physical cpu id: 30 */
          110, 111,     },
    },
    { /* socket id: 2 */
        { /* physical cpu id: 0 */
          112, 113,     },
        { /* physical cpu id: 1 */
          114, 115,     },
        { /* physical cpu id: 2 */
          116, 117,     },
        { /* physical cpu id: 3 */
          118, 119,     },
        { /* physical cpu id: 4 */
          120, 121,     },
        { /* physical cpu id: 5 */
          122, 123,     },
        { /* physical cpu id: 6 */
          124, 125,     },
        { /* physical cpu id: 8 */
          126, 127,     },
        { /* physical cpu id: 9 */
          128, 129,     },
        { /* physical cpu id: 10 */
          130, 131,     },
        { /* physical cpu id: 11 */
          132, 133,     },
        { /* physical cpu id: 12 */
          134, 135,     },
        { /* physical cpu id: 13 */
          136, 137,     },
        { /* physical cpu id: 14 */
          138, 139,     },
        { /* physical cpu id: 16 */
          140, 141,     },
        { /* physical cpu id: 17 */
          142, 143,     },
        { /* physical cpu id: 18 */
          144, 145,     },
        { /* physical cpu id: 19 */
          146, 147,     },
        { /* physical cpu id: 20 */
          148, 149,     },
        { /* physical cpu id: 21 */
          150, 151,     },
        { /* physical cpu id: 22 */
          152, 153,     },
        { /* physical cpu id: 24 */
          154, 155,     },
        { /* physical cpu id: 25 */
          156, 157,     },
        { /* physical cpu id: 26 */
          158, 159,     },
        { /* physical cpu id: 27 */
          160, 161,     },
        { /* physical cpu id: 28 */
          162, 163,     },
        { /* physical cpu id: 29 */
          164, 165,     },
        { /* physical cpu id: 30 */
          166, 167,     },
    },
    { /* socket id: 3 */
        { /* physical cpu id: 0 */
          168, 169,     },
        { /* physical cpu id: 1 */
          170, 171,     },
        { /* physical cpu id: 2 */
          172, 173,     },
        { /* physical cpu id: 3 */
          174, 175,     },
        { /* physical cpu id: 4 */
          176, 177,     },
        { /* physical cpu id: 5 */
          178, 179,     },
        { /* physical cpu id: 6 */
          180, 181,     },
        { /* physical cpu id: 8 */
          182, 183,     },
        { /* physical cpu id: 9 */
          184, 185,     },
        { /* physical cpu id: 10 */
          186, 187,     },
        { /* physical cpu id: 11 */
          188, 189,     },
        { /* physical cpu id: 12 */
          190, 191,     },
        { /* physical cpu id: 13 */
          192, 193,     },
        { /* physical cpu id: 14 */
          194, 195,     },
        { /* physical cpu id: 16 */
          196, 197,     },
        { /* physical cpu id: 17 */
          198, 199,     },
        { /* physical cpu id: 18 */
          200, 201,     },
        { /* physical cpu id: 19 */
          202, 203,     },
        { /* physical cpu id: 20 */
          204, 205,     },
        { /* physical cpu id: 21 */
          206, 207,     },
        { /* physical cpu id: 22 */
          208, 209,     },
        { /* physical cpu id: 24 */
          210, 211,     },
        { /* physical cpu id: 25 */
          212, 213,     },
        { /* physical cpu id: 26 */
          214, 215,     },
        { /* physical cpu id: 27 */
          216, 217,     },
        { /* physical cpu id: 28 */
          218, 219,     },
        { /* physical cpu id: 29 */
          220, 221,     },
        { /* physical cpu id: 30 */
          222, 223,     },
    },
    { /* socket id: 4 */
        { /* physical cpu id: 0 */
          224, 225,     },
        { /* physical cpu id: 1 */
          226, 227,     },
        { /* physical cpu id: 2 */
          228, 229,     },
        { /* physical cpu id: 3 */
          230, 231,     },
        { /* physical cpu id: 4 */
          232, 233,     },
        { /* physical cpu id: 5 */
          234, 235,     },
        { /* physical cpu id: 6 */
          236, 237,     },
        { /* physical cpu id: 8 */
          238, 239,     },
        { /* physical cpu id: 9 */
          240, 241,     },
        { /* physical cpu id: 10 */
          242, 243,     },
        { /* physical cpu id: 11 */
          244, 245,     },
        { /* physical cpu id: 12 */
          246, 247,     },
        { /* physical cpu id: 13 */
          248, 249,     },
        { /* physical cpu id: 14 */
          250, 251,     },
        { /* physical cpu id: 16 */
          252, 253,     },
        { /* physical cpu id: 17 */
          254, 255,     },
        { /* physical cpu id: 18 */
          256, 257,     },
        { /* physical cpu id: 19 */
          258, 259,     },
        { /* physical cpu id: 20 */
          260, 261,     },
        { /* physical cpu id: 21 */
          262, 263,     },
        { /* physical cpu id: 22 */
          264, 265,     },
        { /* physical cpu id: 24 */
          266, 267,     },
        { /* physical cpu id: 25 */
          268, 269,     },
        { /* physical cpu id: 26 */
          270, 271,     },
        { /* physical cpu id: 27 */
          272, 273,     },
        { /* physical cpu id: 28 */
          274, 275,     },
        { /* physical cpu id: 29 */
          276, 277,     },
        { /* physical cpu id: 30 */
          278, 279,     },
    },
    { /* socket id: 5 */
        { /* physical cpu id: 0 */
          280, 281,     },
        { /* physical cpu id: 1 */
          282, 283,     },
        { /* physical cpu id: 2 */
          284, 285,     },
        { /* physical cpu id: 3 */
          286, 287,     },
        { /* physical cpu id: 4 */
          288, 289,     },
        { /* physical cpu id: 5 */
          290, 291,     },
        { /* physical cpu id: 6 */
          292, 293,     },
        { /* physical cpu id: 8 */
          294, 295,     },
        { /* physical cpu id: 9 */
          296, 297,     },
        { /* physical cpu id: 10 */
          298, 299,     },
        { /* physical cpu id: 11 */
          300, 301,     },
        { /* physical cpu id: 12 */
          302, 303,     },
        { /* physical cpu id: 13 */
          304, 305,     },
        { /* physical cpu id: 14 */
          306, 307,     },
        { /* physical cpu id: 16 */
          308, 309,     },
        { /* physical cpu id: 17 */
          310, 311,     },
        { /* physical cpu id: 18 */
          312, 313,     },
        { /* physical cpu id: 19 */
          314, 315,     },
        { /* physical cpu id: 20 */
          316, 317,     },
        { /* physical cpu id: 21 */
          318, 319,     },
        { /* physical cpu id: 22 */
          320, 321,     },
        { /* physical cpu id: 24 */
          322, 323,     },
        { /* physical cpu id: 25 */
          324, 325,     },
        { /* physical cpu id: 26 */
          326, 327,     },
        { /* physical cpu id: 27 */
          328, 329,     },
        { /* physical cpu id: 28 */
          330, 331,     },
        { /* physical cpu id: 29 */
          332, 333,     },
        { /* physical cpu id: 30 */
          334, 335,     },
    },
    { /* socket id: 6 */
        { /* physical cpu id: 0 */
          336, 337,     },
        { /* physical cpu id: 1 */
          338, 339,     },
        { /* physical cpu id: 2 */
          340, 341,     },
        { /* physical cpu id: 3 */
          342, 343,     },
        { /* physical cpu id: 4 */
          344, 345,     },
        { /* physical cpu id: 5 */
          346, 347,     },
        { /* physical cpu id: 6 */
          348, 349,     },
        { /* physical cpu id: 8 */
          350, 351,     },
        { /* physical cpu id: 9 */
          352, 353,     },
        { /* physical cpu id: 10 */
          354, 355,     },
        { /* physical cpu id: 11 */
          356, 357,     },
        { /* physical cpu id: 12 */
          358, 359,     },
        { /* physical cpu id: 13 */
          360, 361,     },
        { /* physical cpu id: 14 */
          362, 363,     },
        { /* physical cpu id: 16 */
          364, 365,     },
        { /* physical cpu id: 17 */
          366, 367,     },
        { /* physical cpu id: 18 */
          368, 369,     },
        { /* physical cpu id: 19 */
          370, 371,     },
        { /* physical cpu id: 20 */
          372, 373,     },
        { /* physical cpu id: 21 */
          374, 375,     },
        { /* physical cpu id: 22 */
          376, 377,     },
        { /* physical cpu id: 24 */
          378, 379,     },
        { /* physical cpu id: 25 */
          380, 381,     },
        { /* physical cpu id: 26 */
          382, 383,     },
        { /* physical cpu id: 27 */
          384, 385,     },
        { /* physical cpu id: 28 */
          386, 387,     },
        { /* physical cpu id: 29 */
          388, 389,     },
        { /* physical cpu id: 30 */
          390, 391,     },
    },
    { /* socket id: 7 */
        { /* physical cpu id: 0 */
          392, 393,     },
        { /* physical cpu id: 1 */
          394, 395,     },
        { /* physical cpu id: 2 */
          396, 397,     },
        { /* physical cpu id: 3 */
          398, 399,     },
        { /* physical cpu id: 4 */
          400, 401,     },
        { /* physical cpu id: 5 */
          402, 403,     },
        { /* physical cpu id: 6 */
          404, 405,     },
        { /* physical cpu id: 8 */
          406, 407,     },
        { /* physical cpu id: 9 */
          408, 409,     },
        { /* physical cpu id: 10 */
          410, 411,     },
        { /* physical cpu id: 11 */
          412, 413,     },
        { /* physical cpu id: 12 */
          414, 415,     },
        { /* physical cpu id: 13 */
          416, 417,     },
        { /* physical cpu id: 14 */
          418, 419,     },
        { /* physical cpu id: 16 */
          420, 421,     },
        { /* physical cpu id: 17 */
          422, 423,     },
        { /* physical cpu id: 18 */
          424, 425,     },
        { /* physical cpu id: 19 */
          426, 427,     },
        { /* physical cpu id: 20 */
          428, 429,     },
        { /* physical cpu id: 21 */
          430, 431,     },
        { /* physical cpu id: 22 */
          432, 433,     },
        { /* physical cpu id: 24 */
          434, 435,     },
        { /* physical cpu id: 25 */
          436, 437,     },
        { /* physical cpu id: 26 */
          438, 439,     },
        { /* physical cpu id: 27 */
          440, 441,     },
        { /* physical cpu id: 28 */
          442, 443,     },
        { /* physical cpu id: 29 */
          444, 445,     },
        { /* physical cpu id: 30 */
          446, 447,     },
    },
};
