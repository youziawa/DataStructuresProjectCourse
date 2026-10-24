/*
 * 目录森林的增删改查、路径回溯、非递归遍历，以及树/森林与二叉树的互转。
 * 目录树的结点定义：
 *   firstChild  长子，转换后是二叉树左孩子
 *   nextSibling 右兄弟，转换后是二叉树右孩子
 *   parent      仅用于把祖先压栈，不参与孩子兄弟的定义
 * 新建结点只挂到父结点的孩子链末尾，因此同一层的顺序就是创建顺序。
 */
#include "catalog.h"

#include "stack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//路径分隔符，win和linux都能用，我有时候用ubuntu改代码，你们别动这个-贺
static int is_sep(char c) {
    return c == '/' || c == '\\';
}

//取最后一个点后面的扩展名
static void copy_ext(char *ext, const char *filename) {
    const char *dot = strrchr(filename, '.');
    ext[0] = '\0';
    if (!dot || dot == filename || dot[1] == '\0') {
        return;
    }
    snprintf(ext, MAX_EXT, "%s", dot + 1);
}

//重命名用的单层名字：非空、短于 MAX_NAME、中间不能再出现分隔符
static int valid_single_name(const char *name) {
    int n;
    if (!name || name[0] == '\0') {
        return 0;//如果名字为空，返回0表示无效
    }
    n = (int)strlen(name);
    if (n <= 0 || n >= MAX_NAME) {
        return 0;//如果名字长度小于等于0或者大于等于MAX_NAME，返回0表示无效
    }
    for (; *name; name++) {
        if (is_sep(*name)) {
            return 0;//如果名字中包含路径分隔符，返回0表示无效
        }
    }
    return 1;
}

//calloc 把三个指针都置成 NULL。目录不记录大小，文件从名字里拆出扩展名
static CSNode *new_cs(const char *name, NodeKind kind, long size) {
    CSNode *n = (CSNode *)calloc(1, sizeof(CSNode));
    if (!n) {
        return NULL;
    }
    snprintf(n->name, MAX_NAME, "%s", name);
    n->kind = kind;
    n->size = kind == NK_FILE ? size : 0;
    if (kind == NK_FILE) {
        copy_ext(n->ext, name);
    }
    return n;
}

// 接到长子链的末尾，保证先创建的孩子排在前面。时间与这一层已有的孩子数成正比。
static void append_child(CSNode *parent, CSNode *node) {
    node->parent = parent;
    node->nextSibling = NULL;
    if (!parent->firstChild) {
        parent->firstChild = node;
        return;
    }
    CSNode *p = parent->firstChild;
    while (p->nextSibling) {
        p = p->nextSibling;
    }
    p->nextSibling = node;
}

// 新的课程根目录是森林里的下一棵树，parent 必须是 NULL。
static void append_root(Forest *f, CSNode *node) {
    node->parent = NULL;
    node->nextSibling = NULL;
    if (!f->firstRoot) {
        f->firstRoot = node;
        return;
    }
    CSNode *p = f->firstRoot;
    while (p->nextSibling) {
        p = p->nextSibling;
    }
    p->nextSibling = node;
}

// 在一条兄弟链上按名字顺序查找。同层查找是 O(孩子数)。
static CSNode *find_sibling(CSNode *first, const char *name) {
    CSNode *p;
    for (p = first; p; p = p->nextSibling) {
        if (strcmp(p->name, name) == 0) {
            return p;
        }
    }
    return NULL;
}

/*
 * 释放 n 以及它的全部子孙，不释放 n 的右兄弟。
 * 先把每个孩子从兄弟链上摘下来再递归，避免顺着 nextSibling 把同层其它结点也释放掉。
 */
static void free_subtree(CSNode *n) {
    CSNode *c;
    if (!n) {
        return;
    }
    c = n->firstChild;
    while (c) {
        CSNode *next = c->nextSibling;
        free_subtree(c);
        c = next;
    }
    free(n);
}

// 把树上的一个文件抄进 FileInfo。路径通过栈回溯生成，不在结点里另存一份。
static void fill_info(FileInfo *e, const CSNode *n) {
    MyString path;
    StrInit(&path);
    catalog_path_string(n, &path);
    fileinfo_make(e, n->name, StrCStr(&path), n->size, n->ext);
    StrDestroy(&path);
}

void forest_init(Forest *f) {
    f->firstRoot = NULL;
}

// 按根的兄弟链逐棵释放。释放前先断开 nextSibling，free_subtree 才不会碰到下一棵树。
void forest_destroy(Forest *f) {
    CSNode *p;
    if (!f) {
        return;
    }
    p = f->firstRoot;
    while (p) {
        CSNode *next = p->nextSibling;
        p->nextSibling = NULL;
        free_subtree(p);
        p = next;
    }
    f->firstRoot = NULL;
}

const char *catalog_err(int code) {
    switch (code) {
    case CAT_OK:
        return "成功";
    case CAT_NOT_FOUND:
        return "路径不存在";
    case CAT_DUP:
        return "同一层已有同名结点";
    case CAT_TYPE:
        return "结点类型不符合要求";
    case CAT_INVALID:
        return "路径或名称不合法";
    case CAT_NOMEM:
        return "内存不足";
    default:
        return "未知错误";
    }
}

/*
 * 用 StrAssign / SubString 拆路径，满足“路径处理必须走串运算”。
 * 开头的分隔符和空格跳过；两个分隔符中间没有名字则判为非法，例如 "a//b"。
 */
int split_path(const char *path, char parts[][MAX_NAME], int max_parts) {
    MyString s;
    MyString sub;
    int n = 0;
    int i = 0;
    if (!path || max_parts <= 0 || strlen(path) >= MAX_PATH) {
        return CAT_INVALID;
    }
    StrInit(&s);
    StrInit(&sub);
    if (StrAssign(&s, path) != 0) {
        StrDestroy(&s);
        StrDestroy(&sub);
        return CAT_NOMEM;
    }
    while (i < s.length && (is_sep(s.data[i]) || s.data[i] == ' ')) {
        i++;
    }
    while (i < s.length) {
        int start, end, len;
        start = i;
        while (i < s.length && !is_sep(s.data[i])) {
            i++;
        }
        end = i;
        while (end > start && s.data[end - 1] == ' ') {
            end--;
        }
        len = end - start;
        if (len <= 0 || len >= MAX_NAME || n >= max_parts) {
            n = CAT_INVALID;
            break;
        }
        if (SubString(&sub, &s, start, len) != 0) {
            n = CAT_NOMEM;
            break;
        }
        memcpy(parts[n], sub.data, (size_t)sub.length + 1);
        n++;
        if (i >= s.length) {
            break;
        }
        i++;
        while (i < s.length && s.data[i] == ' ') {
            i++;
        }
        if (i < s.length && is_sep(s.data[i])) {
            n = CAT_INVALID;
            break;
        }
    }
    if (n == 0) {
        n = CAT_INVALID;
    }
    StrDestroy(&s);
    StrDestroy(&sub);
    return n;
}

// 从森林的根链开始，一层层在兄弟链上找名字。中间遇到文件却还有下一层，直接失败。
CSNode *catalog_find(Forest *f, const char *path) {
    char parts[MAX_DEPTH][MAX_NAME];
    int n, i;
    CSNode *cur;
    if (!f) {
        return NULL;
    }
    n = split_path(path, parts, MAX_DEPTH);
    if (n <= 0) {
        return NULL;
    }
    cur = find_sibling(f->firstRoot, parts[0]);
    if (!cur) {
        return NULL;
    }
    for (i = 1; i < n; i++) {
        if (cur->kind != NK_DIR) {
            return NULL;
        }
        cur = find_sibling(cur->firstChild, parts[i]);
        if (!cur) {
            return NULL;
        }
    }
    return cur;
}

/*
 * 定位路径的父结点，也就是除最后一层以外的那一段。
 * n==1 时没有父结点，返回 NULL 且 err 仍是 CAT_OK，表示要创建的是一棵新树。
 * 中间某一层不存在返回 CAT_NOT_FOUND；中间某一层是文件返回 CAT_TYPE。
 */
static CSNode *find_parent(Forest *f, char parts[][MAX_NAME], int n, int *err) {
    CSNode *cur;
    int i;
    *err = CAT_OK;
    if (n <= 1) {
        return NULL;
    }
    cur = find_sibling(f->firstRoot, parts[0]);
    if (!cur) {
        *err = CAT_NOT_FOUND;
        return NULL;
    }
    for (i = 1; i < n - 1; i++) {
        if (cur->kind != NK_DIR) {
            *err = CAT_TYPE;
            return NULL;
        }
        cur = find_sibling(cur->firstChild, parts[i]);
        if (!cur) {
            *err = CAT_NOT_FOUND;
            return NULL;
        }
    }
    if (cur->kind != NK_DIR) {
        *err = CAT_TYPE;
        return NULL;
    }
    return cur;
}

// 只新建最后一层目录。父路径不存在时不自动补齐，方便测试“路径不存在”。
int catalog_mkdir(Forest *f, const char *path) {
    char parts[MAX_DEPTH][MAX_NAME];
    int n, err;
    CSNode *parent, *node, *exist;
    if (!f) {
        return CAT_INVALID;
    }
    n = split_path(path, parts, MAX_DEPTH);
    if (n <= 0) {
        return n == 0 ? CAT_INVALID : n;
    }
    if (n == 1) {
        if (find_sibling(f->firstRoot, parts[0])) {
            return CAT_DUP;
        }
        node = new_cs(parts[0], NK_DIR, 0);
        if (!node) {
            return CAT_NOMEM;
        }
        append_root(f, node);
        return CAT_OK;
    }
    parent = find_parent(f, parts, n, &err);
    if (!parent) {
        return err;
    }
    exist = find_sibling(parent->firstChild, parts[n - 1]);
    if (exist) {
        return CAT_DUP;
    }
    node = new_cs(parts[n - 1], NK_DIR, 0);
    if (!node) {
        return CAT_NOMEM;
    }
    append_child(parent, node);
    return CAT_OK;
}

// 新建文件。单层路径表示这是森林中的一棵“只有一个文件”的树，用于单文件边界。
int catalog_touch(Forest *f, const char *path, long size) {
    char parts[MAX_DEPTH][MAX_NAME];
    int n, err;
    CSNode *parent, *node;
    if (!f || size < 0) {
        return CAT_INVALID;
    }
    n = split_path(path, parts, MAX_DEPTH);
    if (n <= 0) {
        return n == 0 ? CAT_INVALID : n;
    }
    if (n == 1) {
        if (find_sibling(f->firstRoot, parts[0])) {
            return CAT_DUP;
        }
        node = new_cs(parts[0], NK_FILE, size);
        if (!node) {
            return CAT_NOMEM;
        }
        append_root(f, node);
        return CAT_OK;
    }
    parent = find_parent(f, parts, n, &err);
    if (!parent) {
        return err;
    }
    if (find_sibling(parent->firstChild, parts[n - 1])) {
        return CAT_DUP;
    }
    node = new_cs(parts[n - 1], NK_FILE, size);
    if (!node) {
        return CAT_NOMEM;
    }
    append_child(parent, node);
    return CAT_OK;
}

/*
 * 把结点从兄弟链上摘下，不释放内存。
 * 它若是长子，要改父结点的 firstChild；它若是某棵树的根，要改 forest->firstRoot。
 */
static int unlink_node(Forest *f, CSNode *node) {
    CSNode *prev = NULL;
    CSNode *p;
    CSNode *head = node->parent ? node->parent->firstChild : f->firstRoot;
    for (p = head; p; prev = p, p = p->nextSibling) {
        if (p == node) {
            if (prev) {
                prev->nextSibling = node->nextSibling;
            } else if (node->parent) {
                node->parent->firstChild = node->nextSibling;
            } else {
                f->firstRoot = node->nextSibling;
            }
            node->nextSibling = NULL;
            return 0;
        }
    }
    return -1;
}

// 先摘链再释放子树。若先 free，父结点或左兄弟仍会指向已释放的地址。
int catalog_delete(Forest *f, const char *path) {
    CSNode *node = catalog_find(f, path);
    if (!node) {
        return path && path[0] ? CAT_NOT_FOUND : CAT_INVALID;
    }
    if (unlink_node(f, node) != 0) {
        return CAT_NOT_FOUND;
    }
    free_subtree(node);
    return CAT_OK;
}

// 改名后如果是文件，扩展名按新名字重算，否则按扩展名过滤会仍显示旧后缀。
int catalog_rename(Forest *f, const char *path, const char *new_name) {
    CSNode *node;
    CSNode *head;
    if (!valid_single_name(new_name)) {
        return CAT_INVALID;
    }
    node = catalog_find(f, path);
    if (!node) {
        return CAT_NOT_FOUND;
    }
    if (strcmp(node->name, new_name) == 0) {
        return CAT_OK;
    }
    head = node->parent ? node->parent->firstChild : f->firstRoot;
    if (find_sibling(head, new_name)) {
        return CAT_DUP;
    }
    snprintf(node->name, MAX_NAME, "%s", new_name);
    if (node->kind == NK_FILE) {
        copy_ext(node->ext, new_name);
    } else {
        node->ext[0] = '\0';
    }
    return CAT_OK;
}

int catalog_set_size(Forest *f, const char *path, long size) {
    CSNode *node;
    if (size < 0) {
        return CAT_INVALID;
    }
    node = catalog_find(f, path);
    if (!node) {
        return CAT_NOT_FOUND;
    }
    if (node->kind != NK_FILE) {
        return CAT_TYPE;
    }
    node->size = size;
    return CAT_OK;
}

/*
 * 路径回溯。parent 链的方向是从目标走向根，和要输出的方向相反，
 * 所以先全部入栈，再弹出：栈的后进先出正好把顺序反过来。
 * 各级名字用 StrAppendCStr 接成 "学院/课程/文件"，这是串的连接运算。
 */
int catalog_path_string(const CSNode *node, MyString *out) {
    SeqStack s;
    int first = 1;
    const CSNode *p;
    if (!node || !out) {
        return CAT_INVALID;
    }
    stack_init(&s);
    for (p = node; p; p = p->parent) {
        if (stack_push(&s, (void *)p) != 0) {
            stack_destroy(&s);
            return CAT_NOMEM;
        }
    }
    StrClear(out);
    while (!stack_empty(&s)) {
        const CSNode *cur = (const CSNode *)stack_pop(&s);
        if (!first && StrAppendCStr(out, "/") != 0) {
            stack_destroy(&s);
            return CAT_NOMEM;
        }
        first = 0;
        if (StrAppendCStr(out, cur->name) != 0) {
            stack_destroy(&s);
            return CAT_NOMEM;
        }
    }
    stack_destroy(&s);
    return CAT_OK;
}

// 与 catalog_path_string 同一算法，只是把每一次入栈、出栈打印出来。
void catalog_show_path(const CSNode *node) {
    SeqStack s;
    const CSNode *p;
    MyString path;
    int first = 1;
    if (!node) {
        printf("结点为空。\n");
        return;
    }
    stack_init(&s);
    StrInit(&path);
    printf("使用顺序栈回溯：从目标向根入栈，再依次出栈，得到从根到目标的路径。\n");
    for (p = node; p; p = p->parent) {
        stack_push(&s, (void *)p);
        printf("  入栈：%s\n", p->name);
    }
    while (!stack_empty(&s)) {
        const CSNode *cur = (const CSNode *)stack_pop(&s);
        printf("  出栈：%s\n", cur->name);
        if (!first) {
            StrAppendCStr(&path, "/");
        }
        first = 0;
        StrAppendCStr(&path, cur->name);
    }
    printf("完整路径：%s\n", StrCStr(&path));
    StrDestroy(&path);
    stack_destroy(&s);
}

void catalog_show_node(Forest *f, const char *path) {
    CSNode *n;
    MyString full;
    int children = 0;
    CSNode *c;
    char parts[MAX_DEPTH][MAX_NAME];
    n = catalog_find(f, path);
    if (!n) {
        if (split_path(path, parts, MAX_DEPTH) <= 0) {
            printf("查询失败：%s\n", catalog_err(CAT_INVALID));
        } else {
            printf("查询失败：%s\n", catalog_err(CAT_NOT_FOUND));
        }
        return;
    }
    StrInit(&full);
    catalog_path_string(n, &full);
    printf("名称：%s\n", n->name);
    printf("类型：%s\n", n->kind == NK_DIR ? "目录" : "文件");
    printf("路径：%s\n", StrCStr(&full));
    if (n->kind == NK_FILE) {
        printf("大小：%ld 字节\n", n->size);
        printf("扩展名：%s\n", n->ext[0] ? n->ext : "（无）");
    } else {
        for (c = n->firstChild; c; c = c->nextSibling) {
            children++;
        }
        printf("直接子结点数：%d\n", children);
        if (children == 0) {
            printf("这是空目录。\n");
        }
    }
    StrDestroy(&full);
}

// 先序地按缩进打印：先打印结点，再打印长子子树，然后回到兄弟。仅用于显示。
static void print_tree(const CSNode *n, int depth) {
    const CSNode *p;
    for (p = n; p; p = p->nextSibling) {
        int i;
        for (i = 0; i < depth; i++) {
            printf("  ");
        }
        if (p->kind == NK_DIR) {
            printf("[目录] %s%s\n", p->name, p->firstChild ? "" : "  （空）");
        } else if (p->ext[0]) {
            printf("[文件] %s  大小=%ld  扩展名=%s\n", p->name, p->size, p->ext);
        } else {
            printf("[文件] %s  大小=%ld\n", p->name, p->size);
        }
        print_tree(p->firstChild, depth + 1);
    }
}

void catalog_print_forest(const Forest *f) {
    if (!f || !f->firstRoot) {
        printf("（空森林）\n");
        return;
    }
    print_tree(f->firstRoot, 0);
}

void catalog_show_dir(Forest *f, const char *path) {
    CSNode *n;
    CSNode *c;
    if (!path || path[0] == '\0') {
        printf("当前森林：\n");
        catalog_print_forest(f);
        return;
    }
    n = catalog_find(f, path);
    if (!n) {
        printf("浏览失败：%s\n", catalog_err(CAT_NOT_FOUND));
        return;
    }
    catalog_show_node(f, path);
    if (n->kind != NK_DIR) {
        printf("该结点是文件，没有子目录。\n");
        return;
    }
    printf("直接孩子：\n");
    if (!n->firstChild) {
        printf("  （空目录）\n");
        return;
    }
    for (c = n->firstChild; c; c = c->nextSibling) {
        if (c->kind == NK_DIR) {
            printf("  [目录] %s\n", c->name);
        } else {
            printf("  [文件] %s  大小=%ld\n", c->name, c->size);
        }
    }
}

/*
 * 只收集直接文件孩子。子目录留给树去表达，不放进“同层文件表”。
 * 顺序表尾插、链表尾插，两张表的下标含义相同，后面的查找对比才公平。
 */
int catalog_collect_level(const CSNode *dir, SeqList *files, LinkList *links) {
    const CSNode *c;
    if (!dir || dir->kind != NK_DIR) {
        return dir ? CAT_TYPE : CAT_INVALID;
    }
    if (files) {
        seq_clear(files);
    }
    if (links) {
        link_clear(links);
    }
    for (c = dir->firstChild; c; c = c->nextSibling) {
        FileInfo info;
        if (c->kind != NK_FILE) {
            continue;
        }
        fill_info(&info, c);
        if (files && seq_push(files, &info) != 0) {
            return CAT_NOMEM;
        }
        if (links && link_push_back((LinkList *)links, &info, NULL) != 0) {
            return CAT_NOMEM;
        }
    }
    return CAT_OK;
}

// 文件夹不参与文件名匹配。命中时 pos 是关键词在文件名中的起始下标。
static int match_file(const CSNode *n, const MyString *pat, int *pos, OpStat *st) {
    MyString name;
    int at;
    if (n->kind != NK_FILE) {
        return 0;
    }
    StrInit(&name);
    StrAssign(&name, n->name);
    at = StrIndex(&name, pat, st);
    StrDestroy(&name);
    if (at < 0) {
        return 0;
    }
    if (pos) {
        *pos = at;
    }
    return 1;
}

/*
 * 用顺序栈做深度优先，走完整片森林。先压右兄弟再压长子，和先序遍历的入栈顺序一致。
 * 每个文件名交给 StrIndex 做朴素匹配。
 */
int catalog_collect_keyword(const Forest *f, const char *keyword, SeqList *out) {
    MyString pat;
    SeqStack s;
    if (!out) {
        return CAT_INVALID;
    }
    seq_clear(out);
    if (!keyword || keyword[0] == '\0') {
        return CAT_INVALID;
    }
    StrInit(&pat);
    if (StrAssign(&pat, keyword) != 0) {
        return CAT_NOMEM;
    }
    if (!f || !f->firstRoot) {
        StrDestroy(&pat);
        return CAT_OK;
    }
    stack_init(&s);
    stack_push(&s, f->firstRoot);
    while (!stack_empty(&s)) {
        CSNode *p = (CSNode *)stack_pop(&s);
        if (match_file(p, &pat, NULL, NULL)) {
            FileInfo info;
            fill_info(&info, p);
            seq_push(out, &info);
        }
        if (p->nextSibling) {
            stack_push(&s, p->nextSibling);
        }
        if (p->firstChild) {
            stack_push(&s, p->firstChild);
        }
    }
    stack_destroy(&s);
    StrDestroy(&pat);
    return CAT_OK;
}

int catalog_collect_ext(const Forest *f, const char *ext, SeqList *out) {
    SeqStack s;
    if (!out || !ext) {
        return CAT_INVALID;
    }
    seq_clear(out);
    if (!f || !f->firstRoot) {
        return CAT_OK;
    }
    stack_init(&s);
    stack_push(&s, f->firstRoot);
    while (!stack_empty(&s)) {
        CSNode *p = (CSNode *)stack_pop(&s);
        if (p->kind == NK_FILE && strcmp(p->ext, ext) == 0) {
            FileInfo info;
            fill_info(&info, p);
            seq_push(out, &info);
        }
        if (p->nextSibling) {
            stack_push(&s, p->nextSibling);
        }
        if (p->firstChild) {
            stack_push(&s, p->firstChild);
        }
    }
    stack_destroy(&s);
    return CAT_OK;
}

void catalog_show_keyword(const Forest *f, const char *keyword) {
    MyString pat;
    SeqStack s;
    int hits = 0;
    OpStat total;
    if (!keyword || keyword[0] == '\0') {
        printf("关键词不能为空。\n");
        return;
    }
    StrInit(&pat);
    StrAssign(&pat, keyword);
    stat_reset(&total);
    printf("朴素模式匹配：在每个文件名中逐位对齐关键词。最坏时间 O(m*p)，m 为文件名长度，p 为关键词长度。\n");
    if (f && f->firstRoot) {
        stack_init(&s);
        stack_push(&s, f->firstRoot);
        while (!stack_empty(&s)) {
            CSNode *p = (CSNode *)stack_pop(&s);
            int pos = -1;
            OpStat one;
            stat_reset(&one);
            if (match_file(p, &pat, &pos, &one)) {
                MyString path;
                StrInit(&path);
                catalog_path_string(p, &path);
                printf("命中：%s\n  路径：%s\n  匹配起点：%d  比较次数：%ld\n", p->name, StrCStr(&path), pos, one.compares);
                StrDestroy(&path);
                hits++;
                total.compares += one.compares;
            } else if (p->kind == NK_FILE) {
                total.compares += one.compares;
            }
            if (p->nextSibling) {
                stack_push(&s, p->nextSibling);
            }
            if (p->firstChild) {
                stack_push(&s, p->firstChild);
            }
        }
        stack_destroy(&s);
    }
    printf("共命中 %d 个文件，文件名比较总次数 %ld。\n", hits, total.compares);
    StrDestroy(&pat);
}

void catalog_show_ext(const Forest *f, const char *ext) {
    SeqList list;
    int i;
    if (!ext) {
        printf("扩展名不能为空指针。\n");
        return;
    }
    seq_init(&list);
    catalog_collect_ext(f, ext, &list);
    if (ext[0] == '\0') {
        printf("扩展名为空的文件：\n");
    } else {
        printf("扩展名为 %s 的文件：\n", ext);
    }
    if (list.length == 0) {
        printf("  （没有匹配文件）\n");
    }
    for (i = 0; i < list.length; i++) {
        printf("  %s  大小=%ld  路径=%s\n", list.data[i].name, list.data[i].size, list.data[i].path);
    }
    printf("共 %d 个。\n", list.length);
    seq_destroy(&list);
}

static int count_walk(const CSNode *n) {
    if (!n) {
        return 0;
    }
    return 1 + count_walk(n->firstChild) + count_walk(n->nextSibling);
}

int catalog_node_count(const Forest *f) {
    if (!f) {
        return 0;
    }
    return count_walk(f->firstRoot);
}

// 结构相等：名字、类型、大小、扩展名相同，并且长子链和兄弟链递归相同。不比较指针值。
static int node_equal(const CSNode *a, const CSNode *b) {
    if (!a || !b) {
        return a == b;
    }
    if (strcmp(a->name, b->name) != 0 || a->kind != b->kind || a->size != b->size) {
        return 0;
    }
    if (strcmp(a->ext, b->ext) != 0) {
        return 0;
    }
    return node_equal(a->firstChild, b->firstChild) && node_equal(a->nextSibling, b->nextSibling);
}

int forest_equal(const Forest *a, const Forest *b) {
    if (!a || !b) {
        return 0;
    }
    return node_equal(a->firstRoot, b->firstRoot);
}

// expected 是 n 应该指向的父结点。每个孩子再以 n 作为它们的期望父结点向下检查。
static int parents_ok_node(const CSNode *n, const CSNode *expected) {
    const CSNode *c;
    if (!n) {
        return 1;
    }
    if (n->parent != expected) {
        return 0;
    }
    for (c = n->firstChild; c; c = c->nextSibling) {
        if (!parents_ok_node(c, n)) {
            return 0;
        }
    }
    return 1;
}

int forest_parents_ok(const Forest *f) {
    const CSNode *p;
    if (!f) {
        return 0;
    }
    for (p = f->firstRoot; p; p = p->nextSibling) {
        if (!parents_ok_node(p, NULL)) {
            return 0;
        }
    }
    return 1;
}

/*
 * 把一条兄弟链按相反顺序压入 dst，使下一个弹出的是长子而不是最后一个兄弟。
 * 借助临时栈完成逆置：先顺序压入临时栈，再弹出到目标栈。
 */
static void push_reverse_siblings(SeqStack *dst, const CSNode *first) {
    SeqStack tmp;
    const CSNode *c;
    stack_init(&tmp);
    for (c = first; c; c = c->nextSibling) {
        stack_push(&tmp, (void *)c);
    }
    while (!stack_empty(&tmp)) {
        stack_push(dst, stack_pop(&tmp));
    }
    stack_destroy(&tmp);
}

/*
 * 孩子兄弟链表上的非递归先序，等价于把这条链当成二叉树后的先序。
 * 弹出并访问 p 之后，先压 nextSibling，再压 firstChild。
 * 栈顶是后压入的长子，所以下一轮先处理长子，兄弟留到这棵子树结束之后。
 * 从森林的第一棵根调用时，右兄弟链会把后面的树也走完。
 */
void cs_preorder(const CSNode *root, WalkFn fn, void *ctx) {
    SeqStack s;
    if (!root || !fn) {
        return;
    }
    stack_init(&s);
    stack_push(&s, (void *)root);
    while (!stack_empty(&s)) {
        const CSNode *p = (const CSNode *)stack_pop(&s);
        fn(ctx, p->name);
        // 先压兄弟、后压长子，出栈时才是先长子后兄弟。
        if (p->nextSibling) {
            stack_push(&s, (void *)p->nextSibling);
        }
        if (p->firstChild) {
            stack_push(&s, (void *)p->firstChild);
        }
    }
    stack_destroy(&s);
}

/*
 * 只走一棵一般树的先根次序：访问根，再从左到右访问各子树。
 * 不把 root->nextSibling 压栈，因此不会跑到旁边那棵树。
 */
void cs_tree_preorder(const CSNode *root, WalkFn fn, void *ctx) {
    SeqStack s;
    if (!root || !fn) {
        return;
    }
    stack_init(&s);
    stack_push(&s, (void *)root);
    while (!stack_empty(&s)) {
        const CSNode *p = (const CSNode *)stack_pop(&s);
        fn(ctx, p->name);
        push_reverse_siblings(&s, p->firstChild);
    }
    stack_destroy(&s);
}

/*
 * 一般树后序需要记住“这个结点的孩子处理到哪了”。
 * state==0 表示刚入栈，还没展开孩子；state==1 表示孩子已经入栈，下次轮到它时就可以访问。
 * 用下标而不是指针保存栈顶，是因为继续入栈可能 realloc，原来的指针会失效。
 */
typedef struct {
    const CSNode *node;
    int state; // 0：尚未展开孩子；1：孩子已处理，可以访问自己
} WalkFrame;

typedef struct {
    WalkFrame *data;
    int top;
    int cap;
} FrameStack;

static int fs_push(FrameStack *s, const CSNode *node, int state);

static void push_frames_reverse(FrameStack *fs, const CSNode *first) {
    SeqStack tmp;
    const CSNode *c;
    stack_init(&tmp);
    for (c = first; c; c = c->nextSibling) {
        stack_push(&tmp, (void *)c);
    }
    while (!stack_empty(&tmp)) {
        fs_push(fs, (const CSNode *)stack_pop(&tmp), 0);
    }
    stack_destroy(&tmp);
}

static int fs_push(FrameStack *s, const CSNode *node, int state) {
    if (s->top + 1 >= s->cap) {
        int cap = s->cap > 0 ? s->cap * 2 : 16;
        WalkFrame *nd = (WalkFrame *)realloc(s->data, (size_t)cap * sizeof(WalkFrame));
        if (!nd) {
            return -1;
        }
        s->data = nd;
        s->cap = cap;
    }
    s->top++;
    s->data[s->top].node = node;
    s->data[s->top].state = state;
    return 0;
}

/*
 * 一般树非递归后序。孩子按从左到右的顺序处理完，最后才访问根。
 * 示例 A 的孩子是 B、C 时，序列是 B C A。
 * 同一棵树转成二叉树后的后序是 C B A，因为 C 是 B 的右孩子而不是 A 的第二棵子树。
 */
void cs_tree_postorder(const CSNode *root, WalkFn fn, void *ctx) {
    FrameStack fs;
    if (!root || !fn) {
        return;
    }
    fs.data = NULL;
    fs.top = -1;
    fs.cap = 0;
    fs_push(&fs, root, 0);
    while (fs.top >= 0) {
        int idx = fs.top;
        if (fs.data[idx].state == 0) {
            const CSNode *node = fs.data[idx].node;
            // 先标记成“孩子已展开”，再把孩子压到自己上面。孩子全部出栈后才会回到这里。
            fs.data[idx].state = 1;
            push_frames_reverse(&fs, node->firstChild);
        } else {
            const CSNode *node = fs.data[idx].node;
            fs.top--;
            fn(ctx, node->name);
        }
    }
    free(fs.data);
}

void forest_preorder(const Forest *f, WalkFn fn, void *ctx) {
    if (f) {
        cs_preorder(f->firstRoot, fn, ctx);
    }
}

// 森林的一般树后序是每棵树各自后序，不能对整片森林直接做二叉树后序。
void forest_tree_postorder(const Forest *f, WalkFn fn, void *ctx) {
    const CSNode *p;
    if (!f) {
        return;
    }
    for (p = f->firstRoot; p; p = p->nextSibling) {
        cs_tree_postorder(p, fn, ctx);
    }
}

static void append_line(void *ctx, const char *name) {
    MyString *s = (MyString *)ctx;
    StrAppendCStr(s, name);
    StrAppendCStr(s, "\n");
}

static BiNode *new_bi_from_cs(const CSNode *n) {
    BiNode *b = (BiNode *)calloc(1, sizeof(BiNode));
    if (!b) {
        return NULL;
    }
    snprintf(b->name, MAX_NAME, "%s", n->name);
    b->kind = n->kind;
    b->size = n->size;
    snprintf(b->ext, MAX_EXT, "%s", n->ext);
    return b;
}

/*
 * 树（森林）转二叉树：左孩子复制长子，右孩子复制右兄弟。
 * 递归本身就是“先处理长子子树，再处理兄弟链”。时间 O(n)，新结点 O(n)。
 * 任一子转换失败就把已经建好的部分二叉树释放掉，避免半成品泄漏。
 */
static BiNode *cs_to_bi_node(const CSNode *n) {
    BiNode *b;
    if (!n) {
        return NULL;
    }
    b = new_bi_from_cs(n);
    if (!b) {
        return NULL;
    }
    b->lchild = cs_to_bi_node(n->firstChild);
    b->rchild = cs_to_bi_node(n->nextSibling);
    if ((n->firstChild && !b->lchild) || (n->nextSibling && !b->rchild)) {
        bi_destroy(b);
        return NULL;
    }
    return b;
}

static CSNode *new_cs_from_bi(const BiNode *b) {
    CSNode *n = (CSNode *)calloc(1, sizeof(CSNode));
    if (!n) {
        return NULL;
    }
    snprintf(n->name, MAX_NAME, "%s", b->name);
    n->kind = b->kind;
    n->size = b->size;
    snprintf(n->ext, MAX_EXT, "%s", b->ext);
    return n;
}

/*
 * 二叉树转回树。左孩子成为长子，父指针设为当前结点；
 * 右孩子成为右兄弟，父指针仍然是 parent，而不是当前结点。
 * 根的右链转回去时 parent 传入 NULL，于是它们重新成为森林中的树根。
 */
static CSNode *bi_to_cs_node(const BiNode *b, CSNode *parent) {
    CSNode *n;
    if (!b) {
        return NULL;
    }
    n = new_cs_from_bi(b);
    if (!n) {
        return NULL;
    }
    n->parent = parent;
    n->firstChild = bi_to_cs_node(b->lchild, n); // 左孩子变长子，父结点是自己
    n->nextSibling = bi_to_cs_node(b->rchild, parent); // 右孩子变兄弟，父结点与自己相同
    return n;
}

void bi_destroy(BiNode *n) {
    if (!n) {
        return;
    }
    bi_destroy(n->lchild);
    bi_destroy(n->rchild);
    free(n);
}

BiNode *forest_to_bi(const Forest *f) {
    if (!f) {
        return NULL;
    }
    return cs_to_bi_node(f->firstRoot);
}

int bi_to_forest(const BiNode *root, Forest *out) {
    if (!out) {
        return CAT_INVALID;
    }
    forest_init(out);
    out->firstRoot = bi_to_cs_node(root, NULL);
    if (root && !out->firstRoot) {
        return CAT_NOMEM;
    }
    return CAT_OK;
}

// 非递归先序。先压右再压左，栈顶是左孩子。时间 O(n)，栈深最坏 O(n)。
void bi_preorder(const BiNode *root, WalkFn fn, void *ctx) {
    SeqStack s;
    if (!root || !fn) {
        return;
    }
    stack_init(&s);
    stack_push(&s, (void *)root);
    while (!stack_empty(&s)) {
        const BiNode *p = (const BiNode *)stack_pop(&s);
        fn(ctx, p->name);
        if (p->rchild) {
            stack_push(&s, (void *)p->rchild);
        }
        if (p->lchild) {
            stack_push(&s, (void *)p->lchild);
        }
    }
    stack_destroy(&s);
}

/*
 * 非递归中序：沿左链入栈，左链到底后弹出访问，再去右子树。
 * 在孩子兄弟转换来的二叉树上，中序会把“右兄弟”排到长子和父亲之间，
 * 因此可以用中序核对右指针表示的是兄弟而不是第二棵子树。
 */
void bi_inorder(const BiNode *root, WalkFn fn, void *ctx) {
    SeqStack s;
    const BiNode *p = root;
    if (!fn) {
        return;
    }
    stack_init(&s);
    while (p || !stack_empty(&s)) {
        while (p) {
            stack_push(&s, (void *)p);
            p = p->lchild;
        }
        p = (const BiNode *)stack_pop(&s);
        if (!p) {
            break;
        }
        fn(ctx, p->name);
        p = p->rchild;
    }
    stack_destroy(&s);
}

/*
 * 非递归后序。结点第一次放在栈里时先走左子树；
 * 若右孩子存在且还不是上次访问的结点，就转向右孩子；
 * 否则左右都处理完了，弹出并访问。last 用来区分“右子树还没去”和“右子树刚回来”。
 */
void bi_postorder(const BiNode *root, WalkFn fn, void *ctx) {
    SeqStack s;
    const BiNode *p = root;
    const BiNode *last = NULL;
    if (!fn) {
        return;
    }
    stack_init(&s);
    while (p || !stack_empty(&s)) {
        while (p) {
            stack_push(&s, (void *)p);
            p = p->lchild;
        }
        p = (const BiNode *)stack_peek(&s);
        if (!p) {
            break;
        }
        if (p->rchild && p->rchild != last) {
            // 右子树还没有访问，转向右孩子，当前结点留在栈里。
            p = p->rchild;
        } else {
            stack_pop(&s);
            fn(ctx, p->name);
            last = p;
            p = NULL;
        }
    }
    stack_destroy(&s);
}

void bi_preorder_rec(const BiNode *root, WalkFn fn, void *ctx) {
    if (!root || !fn) {
        return;
    }
    fn(ctx, root->name);
    bi_preorder_rec(root->lchild, fn, ctx);
    bi_preorder_rec(root->rchild, fn, ctx);
}

void bi_inorder_rec(const BiNode *root, WalkFn fn, void *ctx) {
    if (!root || !fn) {
        return;
    }
    bi_inorder_rec(root->lchild, fn, ctx);
    fn(ctx, root->name);
    bi_inorder_rec(root->rchild, fn, ctx);
}

void bi_postorder_rec(const BiNode *root, WalkFn fn, void *ctx) {
    if (!root || !fn) {
        return;
    }
    bi_postorder_rec(root->lchild, fn, ctx);
    bi_postorder_rec(root->rchild, fn, ctx);
    fn(ctx, root->name);
}

// 括号表示：名字(左子, 右子)。没有孩子的叶子不打印括号，空孩子打印 #。
static void print_paren(const BiNode *b) {
    if (!b) {
        printf("#");
        return;
    }
    printf("%s", b->name);
    if (b->lchild || b->rchild) {
        printf("(");
        if (b->lchild) {
            print_paren(b->lchild);
        } else {
            printf("#");
        }
        printf(", ");
        if (b->rchild) {
            print_paren(b->rchild);
        } else {
            printf("#");
        }
        printf(")");
    }
}

static void print_bi_table(const BiNode *b) {
    if (!b) {
        return;
    }
    printf("  %s    左孩子/长子=%s    右孩子/右兄弟=%s\n",
           b->name,
           b->lchild ? b->lchild->name : "（空）",
           b->rchild ? b->rchild->name : "（空）");
    print_bi_table(b->lchild);
    print_bi_table(b->rchild);
}

/*
 * 对照输出。直接先序应与二叉树先序相同；
 * 非递归先序应与递归先序相同；
 * 二叉树后序一般不等于一般树后序，只有没有兄弟的链状树才会碰巧相同。
 */
void catalog_show_traversals(const Forest *f) {
    MyString direct_pre, bi_pre, bi_in, bi_post, tree_post, rec_pre;
    BiNode *bi;
    printf("遍历均用顺序栈实现，不使用递归作为主算法。\n");
    printf("时间都是 O(n)，n 为结点数；栈空间最坏 O(n)，平均与树高同阶。\n");
    printf("孩子兄弟链表上的先序 = 一般树先序 = 转换后二叉树的先序。\n");
    printf("二叉树中序用来核对“左=长子、右=右兄弟”；二叉树后序不等于一般树后序。\n");
    if (!f || !f->firstRoot) {
        printf("当前森林为空，遍历序列为空。\n");
        return;
    }
    StrInit(&direct_pre);
    StrInit(&bi_pre);
    StrInit(&bi_in);
    StrInit(&bi_post);
    StrInit(&tree_post);
    StrInit(&rec_pre);
    forest_preorder(f, append_line, &direct_pre);
    forest_tree_postorder(f, append_line, &tree_post);
    bi = forest_to_bi(f);
    bi_preorder(bi, append_line, &bi_pre);
    bi_inorder(bi, append_line, &bi_in);
    bi_postorder(bi, append_line, &bi_post);
    bi_preorder_rec(bi, append_line, &rec_pre);
    printf("\n直接在孩子兄弟树上的先序：\n%s", StrCStr(&direct_pre));
    printf("转换成二叉树后的先序：\n%s", StrCStr(&bi_pre));
    printf("先序是否一致：%s\n", StrCompare(&direct_pre, &bi_pre) == 0 ? "是" : "否");
    printf("非递归先序与递归先序是否一致：%s\n", StrCompare(&bi_pre, &rec_pre) == 0 ? "是" : "否");
    printf("\n二叉树中序（核对左右孩子）：\n%s", StrCStr(&bi_in));
    printf("二叉树后序：\n%s", StrCStr(&bi_post));
    printf("一般树后序（先各子树，再根）：\n%s", StrCStr(&tree_post));
    printf("二叉树后序与一般树后序是否相同：%s\n", StrCompare(&bi_post, &tree_post) == 0 ? "是" : "否");
    bi_destroy(bi);
    StrDestroy(&direct_pre);
    StrDestroy(&bi_pre);
    StrDestroy(&bi_in);
    StrDestroy(&bi_post);
    StrDestroy(&tree_post);
    StrDestroy(&rec_pre);
}

// 转换演示不修改原森林：forest_to_bi 只读结点，bi_to_forest 写到另一片新森林。
void catalog_show_conversion(const Forest *f) {
    BiNode *bi;
    Forest back;
    int same, parents;
    printf("\n对应关系：二叉树左孩子 = 孩子兄弟链表的 firstChild（长子）；右孩子 = nextSibling（右兄弟）。\n");
    printf("森林中下一棵树是根结点的右兄弟，转换后成为二叉树根的右子树。\n");
    printf("直接遍历孩子兄弟树不复制结点，额外空间只有遍历栈 O(h)。\n");
    printf("先复制成二叉树再遍历需要额外 O(n) 时间和空间，遍历本身仍是 O(n)。\n");
    printf("\n【当前森林 / 孩子兄弟树】\n");
    catalog_print_forest(f);
    bi = forest_to_bi(f);
    printf("\n【转换为二叉树】括号表示：结点(左子, 右子)，# 表示空。\n");
    print_paren(bi);
    printf("\n\n【每个结点的左右孩子】\n");
    if (!bi) {
        printf("  （空）\n");
    } else {
        print_bi_table(bi);
    }
    if (bi_to_forest(bi, &back) != CAT_OK) {
        printf("转回森林失败。\n");
        bi_destroy(bi);
        return;
    }
    printf("\n【再转回森林】\n");
    catalog_print_forest(&back);
    same = forest_equal(f, &back);
    parents = forest_parents_ok(&back);
    printf("可逆验证：%s\n", same ? "通过，转回的森林与原森林结构一致" : "失败");
    printf("父指针检查：%s\n", parents ? "通过" : "失败");
    forest_destroy(&back);
    bi_destroy(bi);
}

void catalog_show_mapping_example(void) {
    Forest demo;
    printf("======== 左右指针含义示例 ========\n");
    printf("示例树 A，长子为 B，B 的右兄弟为 C。\n");
    forest_init(&demo);
    catalog_mkdir(&demo, "A");
    catalog_touch(&demo, "A/B", 1);
    catalog_touch(&demo, "A/C", 1);
    catalog_show_traversals(&demo);
    catalog_show_conversion(&demo);
    printf("这个例子里一般树后序是 B C A，二叉树后序是 C B A，两者不同。\n");
    printf("中序 B C A 把兄弟 C 排在 B 和根 A 之间，因为 C 是 B 的右孩子。\n");
    forest_destroy(&demo);
}

/*
 * 示例含题目要求的边界：空目录、多级路径、两棵树组成的森林、
 * 不同文件夹下的同名文件“习题.pdf”。每一层都显式创建，不使用递归建目录。
 */
int catalog_load_sample(Forest *f) {
    static const char *dirs[] = {
        "计算机学院",
        "计算机学院/数据结构",
        "计算机学院/数据结构/第3章",
        "计算机学院/数据结构/第4章",
        "计算机学院/数据结构/空目录",
        "计算机学院/操作系统",
        "信息学院",
        "信息学院/计算机网络",
        NULL
    };
    int i, rc;
    if (!f) {
        return CAT_INVALID;
    }
    forest_destroy(f);
    forest_init(f);
    for (i = 0; dirs[i]; i++) {
        rc = catalog_mkdir(f, dirs[i]);
        if (rc != CAT_OK) {
            return rc;
        }
    }
    rc = catalog_touch(f, "计算机学院/数据结构/第3章/习题.pdf", 120);
    if (rc != CAT_OK) {
        return rc;
    }
    rc = catalog_touch(f, "计算机学院/数据结构/第3章/课件.pptx", 2048);
    if (rc != CAT_OK) {
        return rc;
    }
    rc = catalog_touch(f, "计算机学院/数据结构/第4章/实验.pdf", 300);
    if (rc != CAT_OK) {
        return rc;
    }
    rc = catalog_touch(f, "计算机学院/操作系统/进程.md", 80);
    if (rc != CAT_OK) {
        return rc;
    }
    rc = catalog_touch(f, "计算机学院/操作系统/习题.pdf", 99);
    if (rc != CAT_OK) {
        return rc;
    }
    rc = catalog_touch(f, "信息学院/计算机网络/实验一.pdf", 100);
    if (rc != CAT_OK) {
        return rc;
    }
    return CAT_OK;
}
