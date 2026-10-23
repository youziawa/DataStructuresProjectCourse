/*
 * 自测按模块分组：串、栈和线性表、排序查找、目录增删改查、遍历与互转、示例边界。
 * 断言信息使用英文，是为了在任何控制台编码下都能看出 PASS/FAIL；
 * 被测数据本身包含中文路径和“习题”关键词。
 */
#include "tests.h"

#include "catalog.h"
#include "hashtable.h"
#include "linklist.h"
#include "mystr.h"
#include "sortsearch.h"
#include "stack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_pass = 0;
static int g_fail = 0;

static void expect(int cond, const char *msg) {
    if (cond) {
        g_pass++;
        printf("[PASS] %s\n", msg);
    } else {
        g_fail++;
        printf("[FAIL] %s\n", msg);
    }
}

static void append_line(void *ctx, const char *name) {
    MyString *s = (MyString *)ctx;
    StrAppendCStr(s, name);
    StrAppendCStr(s, "\n");
}

static int newline_count(const MyString *s) {
    int i, n = 0;
    if (!s || !s->data) {
        return 0;
    }
    for (i = 0; i < s->length; i++) {
        if (s->data[i] == '\n') {
            n++;
        }
    }
    return n;
}

/* 串：赋值、朴素匹配（含中文关键词）、空模式、子串、连接。 */
static void test_string(void) {
    MyString a, b, c, sub;
    OpStat st;
    StrInit(&a);
    StrInit(&b);
    StrInit(&c);
    StrInit(&sub);
    expect(StrAssign(&a, "abcabd") == 0, "string assign");
    expect(StrAssign(&b, "abd") == 0, "string assign pattern");
    stat_reset(&st);
    expect(StrIndex(&a, &b, &st) == 3, "BF index of abd");
    expect(st.compares > 0, "BF counts compares");
    StrAssign(&a, "aaaaa");
    StrAssign(&b, "aaa");
    expect(StrIndex(&a, &b, NULL) == 0, "BF overlapping prefix");
    StrAssign(&b, "xyz");
    expect(StrIndex(&a, &b, NULL) == -1, "BF not found");
    StrClear(&b);
    expect(StrIndex(&a, &b, NULL) == 0, "empty pattern index 0");
    StrAssign(&a, "数据结构习题");
    StrAssign(&b, "习题");
    expect(StrIndex(&a, &b, NULL) >= 0, "BF chinese keyword");
    StrAssign(&a, "abcdef");
    expect(SubString(&sub, &a, 2, 3) == 0, "substring ok");
    expect(strcmp(StrCStr(&sub), "cde") == 0, "substring value");
    StrAssign(&a, "ab");
    StrAssign(&b, "cd");
    expect(StrConcat(&c, &a, &b) == 0, "concat ok");
    expect(strcmp(StrCStr(&c), "abcd") == 0, "concat value");
    StrAssign(&a, "root");
    StrAppendCStr(&a, "/");
    StrAppendCStr(&a, "child");
    expect(strcmp(StrCStr(&a), "root/child") == 0, "path concat");
    StrDestroy(&a);
    StrDestroy(&b);
    StrDestroy(&c);
    StrDestroy(&sub);
}

/* 栈的后进先出，以及表头插入 n 次时顺序表、链表的赋值次数公式。 */
static void test_stack_and_lists(void) {
    SeqStack s;
    SeqList seq;
    LinkList link;
    OpStat st;
    FileInfo e;
    int i;
    stack_init(&s);
    expect(stack_empty(&s), "stack starts empty");
    stack_push(&s, (void *)"A");
    stack_push(&s, (void *)"B");
    expect(strcmp((char *)stack_pop(&s), "B") == 0, "stack pop LIFO");
    expect(strcmp((char *)stack_peek(&s), "A") == 0, "stack peek");
    stack_destroy(&s);

    seq_init(&seq);
    link_init(&link);
    stat_reset(&st);
    for (i = 0; i < 10; i++) {
        char name[32];
        snprintf(name, sizeof(name), "n%d", i);
        fileinfo_make(&e, name, name, i, "");
        if (seq_insert(&seq, 0, &e, &st) != 0) {
            expect(0, "seq insert head");
        }
    }
    expect(seq.length == 10, "seq insert head");
    expect(st.moves == 10 * 11 / 2, "seq head insert move count");
    stat_reset(&st);
    for (i = 0; i < 10; i++) {
        char name[32];
        snprintf(name, sizeof(name), "n%d", i);
        fileinfo_make(&e, name, name, i, "");
        if (link_insert_head(&link, &e, &st) != 0) {
            expect(0, "link insert head");
        }
    }
    expect(st.moves == 10, "link head insert move count");
    expect(link_length(&link) == 10, "link length");
    stat_reset(&st);
    expect(link_search(&link, "n0", &st) == 9, "link search last inserted at tail side");
    expect(st.compares == 10, "link search compares");
    seq_destroy(&seq);
    link_destroy(&link);
}

static void names_of(const SeqList *L, char *buf, int nbuf) {
    int i, used = 0;
    buf[0] = '\0';
    for (i = 0; i < L->length; i++) {
        int k = snprintf(buf + used, (size_t)(nbuf - used), "%s%s", i ? "," : "", L->data[i].name);
        if (k < 0 || used + k >= nbuf) {
            return;
        }
        used += k;
    }
}

/* 五种排序的结果和稳定性，以及顺序、折半、分块查找和散列表的重名链。 */
static void test_sort_search(void) {
    SeqList src, a, b, c, d, e;
    OpStat st;
    char na[256], nb[256], nc[256], nd[256], ne[256];
    int i, trial;
    FileInfo info;
    HashTable ht;
    SeqList hits;

    seq_init(&src);
    fileinfo_make(&info, "c", "c", 3, "");
    seq_push(&src, &info);
    fileinfo_make(&info, "a", "a", 1, "");
    seq_push(&src, &info);
    fileinfo_make(&info, "b", "b", 2, "");
    seq_push(&src, &info);

    seq_init(&a);
    seq_init(&b);
    seq_init(&c);
    seq_init(&d);
    seq_init(&e);
    seq_assign(&a, &src);
    seq_assign(&b, &src);
    seq_assign(&c, &src);
    seq_assign(&d, &src);
    seq_assign(&e, &src);
    insert_sort(&a, KEY_NAME, NULL);
    bin_insert_sort(&b, KEY_NAME, NULL);
    shell_sort(&c, KEY_NAME, NULL);
    bubble_sort(&d, KEY_NAME, NULL);
    quick_sort(&e, KEY_NAME, NULL);
    names_of(&a, na, sizeof(na));
    names_of(&b, nb, sizeof(nb));
    names_of(&c, nc, sizeof(nc));
    names_of(&d, nd, sizeof(nd));
    names_of(&e, ne, sizeof(ne));
    expect(strcmp(na, "a,b,c") == 0, "insert sort names");
    expect(strcmp(na, nb) == 0 && strcmp(na, nc) == 0 && strcmp(na, nd) == 0 && strcmp(na, ne) == 0,
           "five sorts agree on unique names");

    seq_clear(&src);
    fileinfo_make(&info, "a", "a", 5, "");
    seq_push(&src, &info);
    fileinfo_make(&info, "b", "b", 4, "");
    seq_push(&src, &info);
    fileinfo_make(&info, "c", "c", 3, "");
    seq_push(&src, &info);
    fileinfo_make(&info, "d", "d", 2, "");
    seq_push(&src, &info);
    fileinfo_make(&info, "e", "e", 1, "");
    seq_push(&src, &info);
    seq_assign(&d, &src);
    stat_reset(&st);
    bubble_sort(&d, KEY_SIZE, &st);
    expect(st.compares == 10, "bubble reverse compare count");
    expect(is_sorted(&d, KEY_SIZE), "bubble reverse becomes sorted");
    names_of(&d, nd, sizeof(nd));
    expect(strcmp(nd, "e,d,c,b,a") == 0, "bubble is stable");

    seq_clear(&src);
    for (i = 0; i < 5; i++) {
        char name[8];
        snprintf(name, sizeof(name), "%c", 'a' + i);
        fileinfo_make(&info, name, name, i, "");
        seq_push(&src, &info);
    }
    seq_assign(&a, &src);
    stat_reset(&st);
    insert_sort(&a, KEY_NAME, &st);
    expect(st.compares == 4, "insert sort best compares");
    expect(st.moves == 8, "insert sort best moves");
    seq_assign(&b, &src);
    stat_reset(&st);
    bin_insert_sort(&b, KEY_NAME, &st);
    expect(is_sorted(&b, KEY_NAME), "binary insert keeps sorted");
    expect(st.moves == 8, "binary insert best moves");
    expect(st.compares > 4, "binary insert still searches");

    seq_clear(&src);
    fileinfo_make(&info, "a", "a", 1, "");
    seq_push(&src, &info);
    fileinfo_make(&info, "b", "b", 2, "");
    seq_push(&src, &info);
    fileinfo_make(&info, "c", "c", 1, "");
    seq_push(&src, &info);
    seq_assign(&a, &src);
    seq_assign(&b, &src);
    seq_assign(&d, &src);
    insert_sort(&a, KEY_SIZE, NULL);
    bin_insert_sort(&b, KEY_SIZE, NULL);
    bubble_sort(&d, KEY_SIZE, NULL);
    names_of(&a, na, sizeof(na));
    names_of(&b, nb, sizeof(nb));
    names_of(&d, nd, sizeof(nd));
    expect(strcmp(na, "a,c,b") == 0, "insert sort stable on size");
    expect(strcmp(nb, "a,c,b") == 0, "binary insert stable on size");
    expect(strcmp(nd, "a,c,b") == 0, "bubble stable on size");

    for (trial = 0; trial < 20; trial++) {
        seq_clear(&src);
        for (i = 0; i < 20; i++) {
            char name[16];
            snprintf(name, sizeof(name), "f%02d", i);
            fileinfo_make(&info, name, name, (trial * 17 + i * 13) % 97, "");
            seq_push(&src, &info);
        }
        seq_assign(&a, &src);
        seq_assign(&b, &src);
        seq_assign(&c, &src);
        seq_assign(&d, &src);
        seq_assign(&e, &src);
        insert_sort(&a, KEY_SIZE, NULL);
        bin_insert_sort(&b, KEY_SIZE, NULL);
        shell_sort(&c, KEY_SIZE, NULL);
        bubble_sort(&d, KEY_SIZE, NULL);
        quick_sort(&e, KEY_SIZE, NULL);
        if (!is_sorted(&a, KEY_SIZE) || !is_sorted(&b, KEY_SIZE) || !is_sorted(&c, KEY_SIZE) ||
            !is_sorted(&d, KEY_SIZE) || !is_sorted(&e, KEY_SIZE)) {
            break;
        }
    }
    expect(trial == 20, "random size sorts all ordered");

    seq_clear(&src);
    for (i = 0; i < 7; i++) {
        char name[8];
        snprintf(name, sizeof(name), "%c", 'a' + i);
        fileinfo_make(&info, name, name, i, "");
        seq_push(&src, &info);
    }
    stat_reset(&st);
    expect(seq_search(&src, "d", &st) == 3, "sequential search hit");
    expect(st.compares == 4, "sequential search compares");
    stat_reset(&st);
    expect(bin_search(&src, "d", &st) == 3, "binary search hit");
    expect(st.compares > 0 && st.compares <= 3, "binary search compare bound");
    stat_reset(&st);
    expect(block_search(&src, "g", 3, &st) == 6, "block search last");
    expect(block_search(&src, "missing", 3, NULL) == -1, "block search miss");
    seq_assign(&a, &src);
    a.data[0].name[0] = 'z';
    expect(bin_search(&a, "a", NULL) == -2, "binary search rejects unsorted");
    expect(block_search(&a, "a", 3, NULL) == -2, "block search rejects unsorted");

    hash_init(&ht, 17);
    seq_init(&hits);
    fileinfo_make(&info, "习题.pdf", "p1", 1, "pdf");
    hash_insert(&ht, &info);
    fileinfo_make(&info, "习题.pdf", "p2", 2, "pdf");
    hash_insert(&ht, &info);
    fileinfo_make(&info, "other.txt", "p3", 3, "txt");
    hash_insert(&ht, &info);
    expect(hash_search_all(&ht, "习题.pdf", &hits, NULL) == 2, "hash keeps duplicate names");
    expect(strcmp(hits.data[0].path, "p2") == 0 || strcmp(hits.data[1].path, "p2") == 0, "hash returns both paths");
    expect(hash_search(&ht, "missing", NULL, NULL) == -1, "hash miss");

    seq_destroy(&src);
    seq_destroy(&a);
    seq_destroy(&b);
    seq_destroy(&c);
    seq_destroy(&d);
    seq_destroy(&e);
    seq_destroy(&hits);
    hash_destroy(&ht);
}

/* 空目录、单文件、父路径不存在、同层重名、删除子树、改名后扩展名变化。 */
static void test_split_and_crud(void) {
    char parts[MAX_DEPTH][MAX_NAME];
    Forest f;
    CSNode *n;
    MyString path;
    SeqList files;
    LinkList links;
    char long_name[160];

    expect(split_path("a/b/c", parts, MAX_DEPTH) == 3, "split three");
    expect(strcmp(parts[0], "a") == 0 && strcmp(parts[2], "c") == 0, "split values");
    expect(split_path("a//b", parts, MAX_DEPTH) == CAT_INVALID, "reject empty component");
    expect(split_path("/a/b/", parts, MAX_DEPTH) == 2, "trim slashes");
    expect(strcmp(parts[0], "a") == 0 && strcmp(parts[1], "b") == 0, "trimmed parts");
    expect(split_path("  a / b ", parts, MAX_DEPTH) == 2, "trim spaces");
    expect(split_path("", parts, MAX_DEPTH) == CAT_INVALID, "empty path");

    forest_init(&f);
    expect(catalog_mkdir(&f, "Empty") == CAT_OK, "empty directory create");
    n = catalog_find(&f, "Empty");
    expect(n && n->kind == NK_DIR && n->firstChild == NULL, "empty directory has no child");
    expect(catalog_node_count(&f) == 1, "empty directory node count");
    forest_destroy(&f);

    forest_init(&f);
    expect(catalog_touch(&f, "only.txt", 5) == CAT_OK, "single root file");
    n = catalog_find(&f, "only.txt");
    expect(n && n->kind == NK_FILE && n->size == 5, "single file fields");
    StrInit(&path);
    expect(catalog_path_string(n, &path) == CAT_OK, "path of root file");
    expect(strcmp(StrCStr(&path), "only.txt") == 0, "root file path text");
    expect(catalog_touch(&f, "only.txt/x", 1) == CAT_TYPE, "cannot create under a file");
    expect(catalog_mkdir(&f, "nope/child") == CAT_NOT_FOUND, "missing parent");
    expect(catalog_find(&f, "missing") == NULL, "missing path");
    expect(catalog_delete(&f, "missing") == CAT_NOT_FOUND, "delete missing");
    forest_destroy(&f);
    StrDestroy(&path);

    forest_init(&f);
    expect(catalog_mkdir(&f, "A") == CAT_OK, "mkdir A");
    expect(catalog_mkdir(&f, "A") == CAT_DUP, "duplicate root");
    expect(catalog_touch(&f, "A/B", 1) == CAT_OK, "touch B");
    expect(catalog_touch(&f, "A/C", 2) == CAT_OK, "touch C");
    expect(catalog_touch(&f, "A/B", 3) == CAT_DUP, "duplicate sibling");
    expect(catalog_mkdir(&f, "D") == CAT_OK, "second tree");
    expect(catalog_touch(&f, "D/B", 4) == CAT_OK, "same name in another folder");
    seq_init(&files);
    link_init(&links);
    n = catalog_find(&f, "A");
    expect(catalog_collect_level(n, &files, &links) == CAT_OK, "collect level");
    expect(files.length == 2 && links.length == 2, "level file count");
    expect(strcmp(files.data[0].name, "B") == 0 && strcmp(files.data[1].name, "C") == 0, "level order");
    expect(catalog_delete(&f, "A/B") == CAT_OK, "delete first child");
    expect(catalog_find(&f, "A/B") == NULL, "deleted path gone");
    expect(catalog_find(&f, "A/C") != NULL, "sibling remains");
    expect(catalog_find(&f, "D/B") != NULL, "other tree untouched");
    expect(catalog_rename(&f, "A/C", "C2") == CAT_OK, "rename");
    n = catalog_find(&f, "A/C2");
    expect(n != NULL, "renamed path");
    expect(catalog_touch(&f, "A/note.txt", 8) == CAT_OK, "touch note");
    expect(catalog_rename(&f, "A/note.txt", "note.md") == CAT_OK, "rename changes ext");
    n = catalog_find(&f, "A/note.md");
    expect(n && strcmp(n->ext, "md") == 0, "extension updated");
    expect(catalog_set_size(&f, "A/note.md", 20) == CAT_OK, "set size");
    expect(n->size == 20, "size updated");
    expect(catalog_set_size(&f, "A", 1) == CAT_TYPE, "dir has no size");
    expect(catalog_rename(&f, "A/note.md", "C2") == CAT_DUP, "rename conflict");
    expect(catalog_delete(&f, "A") == CAT_OK, "delete directory tree");
    expect(catalog_find(&f, "A/note.md") == NULL, "descendants removed");
    expect(catalog_find(&f, "D") != NULL, "other root remains");
    memset(long_name, 'a', 128);
    long_name[128] = '\0';
    expect(catalog_mkdir(&f, long_name) == CAT_INVALID, "reject overlong name");
    long_name[127] = '\0';
    expect(catalog_mkdir(&f, long_name) == CAT_OK, "accept 127-byte name");
    seq_destroy(&files);
    link_destroy(&links);
    forest_destroy(&f);
}

/*
 * A 的孩子是 B、C。先序都是 A B C，一般树后序是 B C A，二叉树后序是 C B A。
 * 再加上第二棵树 D，检查森林互转后父指针和结点是否一一对应。
 */
static void test_traverse_and_convert(void) {
    Forest f, back;
    BiNode *bi;
    MyString pre, inord, post, tree_post, rec, direct;
    int nodes;

    forest_init(&f);
    catalog_mkdir(&f, "A");
    catalog_touch(&f, "A/B", 1);
    catalog_touch(&f, "A/C", 1);
    StrInit(&pre);
    StrInit(&inord);
    StrInit(&post);
    StrInit(&tree_post);
    StrInit(&rec);
    StrInit(&direct);
    forest_preorder(&f, append_line, &direct);
    bi = forest_to_bi(&f);
    bi_preorder(bi, append_line, &pre);
    bi_inorder(bi, append_line, &inord);
    bi_postorder(bi, append_line, &post);
    forest_tree_postorder(&f, append_line, &tree_post);
    bi_preorder_rec(bi, append_line, &rec);
    expect(strcmp(StrCStr(&direct), "A\nB\nC\n") == 0, "tree preorder A B C");
    expect(strcmp(StrCStr(&pre), "A\nB\nC\n") == 0, "binary preorder matches");
    expect(strcmp(StrCStr(&inord), "B\nC\nA\n") == 0, "binary inorder checks sibling");
    expect(strcmp(StrCStr(&post), "C\nB\nA\n") == 0, "binary postorder");
    expect(strcmp(StrCStr(&tree_post), "B\nC\nA\n") == 0, "general tree postorder differs");
    expect(strcmp(StrCStr(&pre), StrCStr(&rec)) == 0, "recursive preorder matches stack");
    StrClear(&rec);
    bi_inorder_rec(bi, append_line, &rec);
    expect(strcmp(StrCStr(&inord), StrCStr(&rec)) == 0, "recursive inorder matches");
    StrClear(&rec);
    bi_postorder_rec(bi, append_line, &rec);
    expect(strcmp(StrCStr(&post), StrCStr(&rec)) == 0, "recursive postorder matches");
    expect(bi_to_forest(bi, &back) == CAT_OK, "convert back");
    expect(forest_equal(&f, &back), "roundtrip equal");
    expect(forest_parents_ok(&back), "parents rebuilt");
    forest_destroy(&back);
    bi_destroy(bi);
    expect(catalog_find(&f, "A/C") != NULL, "original forest kept");

    catalog_mkdir(&f, "D");
    catalog_touch(&f, "D/E", 1);
    nodes = catalog_node_count(&f);
    StrClear(&direct);
    StrClear(&pre);
    StrClear(&inord);
    StrClear(&post);
    StrClear(&tree_post);
    forest_preorder(&f, append_line, &direct);
    bi = forest_to_bi(&f);
    bi_preorder(bi, append_line, &pre);
    bi_inorder(bi, append_line, &inord);
    bi_postorder(bi, append_line, &post);
    forest_tree_postorder(&f, append_line, &tree_post);
    expect(strcmp(StrCStr(&direct), StrCStr(&pre)) == 0, "forest preorder matches binary");
    expect(newline_count(&pre) == nodes, "preorder visits each node");
    expect(newline_count(&inord) == nodes, "inorder visits each node");
    expect(newline_count(&post) == nodes, "postorder visits each node");
    expect(newline_count(&tree_post) == nodes, "tree postorder visits each node");
    expect(bi_to_forest(bi, &back) == CAT_OK, "forest roundtrip");
    expect(forest_equal(&f, &back) && forest_parents_ok(&back), "two-tree forest reversible");
    forest_destroy(&back);
    bi_destroy(bi);
    forest_destroy(&f);

    forest_init(&f);
    bi = forest_to_bi(&f);
    expect(bi == NULL, "empty forest to empty binary tree");
    expect(bi_to_forest(bi, &back) == CAT_OK && forest_equal(&f, &back), "empty roundtrip");
    forest_destroy(&back);
    forest_destroy(&f);

    StrDestroy(&pre);
    StrDestroy(&inord);
    StrDestroy(&post);
    StrDestroy(&tree_post);
    StrDestroy(&rec);
    StrDestroy(&direct);
}

/* 示例数据：空目录、栈生成的完整路径、跨目录重名、关键词、扩展名、转换可逆且不破坏原森林。 */
static void test_sample_boundaries(void) {
    Forest f, back;
    BiNode *bi;
    SeqList files, hits;
    CSNode *n;
    MyString path;
    int pdfs;

    forest_init(&f);
    expect(catalog_load_sample(&f) == CAT_OK, "load sample");
    expect(catalog_node_count(&f) == 14, "sample node count");
    n = catalog_find(&f, "计算机学院/数据结构/空目录");
    expect(n && n->kind == NK_DIR && n->firstChild == NULL, "sample empty directory");
    n = catalog_find(&f, "计算机学院/数据结构/第3章/习题.pdf");
    StrInit(&path);
    catalog_path_string(n, &path);
    expect(n && strcmp(StrCStr(&path), "计算机学院/数据结构/第3章/习题.pdf") == 0, "stack path");
    expect(catalog_find(&f, "计算机学院/操作系统/习题.pdf") != NULL, "duplicate name in another folder");
    expect(catalog_find(&f, "计算机学院/没有这门课") == NULL, "sample missing path");
    seq_init(&files);
    seq_init(&hits);
    expect(catalog_collect_keyword(&f, "习题", &files) == CAT_OK, "keyword search");
    expect(files.length == 2, "keyword hits both files");
    expect(catalog_collect_keyword(&f, "", &files) == CAT_INVALID, "empty keyword rejected");
    expect(catalog_collect_ext(&f, "pdf", &hits) == CAT_OK, "filter pdf");
    pdfs = hits.length;
    expect(pdfs == 4, "four pdf files");
    expect(catalog_collect_ext(&f, "pptx", &hits) == CAT_OK && hits.length == 1, "filter pptx");
    n = catalog_find(&f, "计算机学院/数据结构/第3章");
    expect(catalog_collect_level(n, &files, NULL) == CAT_OK, "chapter level");
    expect(files.length == 2, "two files in chapter 3");
    n = catalog_find(&f, "计算机学院/数据结构");
    expect(catalog_collect_level(n, &files, NULL) == CAT_OK && files.length == 0, "directory level has folders only");
    bi = forest_to_bi(&f);
    expect(bi_to_forest(bi, &back) == CAT_OK, "sample convert back");
    expect(forest_equal(&f, &back), "sample reversible");
    expect(forest_parents_ok(&back), "sample parents");
    expect(catalog_node_count(&f) == 14, "source forest unchanged");
    forest_destroy(&back);
    bi_destroy(bi);
    seq_destroy(&files);
    seq_destroy(&hits);
    StrDestroy(&path);
    forest_destroy(&f);
}

int run_all_tests(void) {
    g_pass = 0;
    g_fail = 0;
    printf("==== string ====\n");
    test_string();
    printf("==== stack and lists ====\n");
    test_stack_and_lists();
    printf("==== sort and search ====\n");
    test_sort_search();
    printf("==== crud and boundaries ====\n");
    test_split_and_crud();
    printf("==== traverse and convert ====\n");
    test_traverse_and_convert();
    printf("==== sample ====\n");
    test_sample_boundaries();
    printf("TOTAL pass=%d fail=%d\n", g_pass, g_fail);
    return g_fail;
}
