/*
 * 2026年数据结构项目课
 * 选题：项目7：校园课程资料目录树与路径检索系统
 * 分工：贺梓健-联调，森林与二叉树转换；张愉哲-串与朴素模式匹配；吴军忌-线性表与顺序查找；李家豪-栈与非递归遍历；赵栋-录树增删改查；曹毅诚-查找与排序；
 * 必须完成的内容：文件夹 / 文件的增、删、改、查；对比；分析树-二叉树转换、三种遍历、串匹配、查找与排序的时间 / 空间复杂度
 * 加分项：同一森林（多个课程根目录）转换为二叉树后再转回森林，验证转换可逆；按扩展名过滤列出文件
 *                 _ooOoo_
                  o8888888o
                  88" . "88
                  (| -_- |)
                  O\  =  /O
               ____/`---'\____
             .'  \\|     |//  `.
            /  \\|||  :  |||//  \
           /  _||||| -:- |||||-  \
           |   | \\\  -  /// |   |
           | \_|  ''\---/''  |   |
           \  .-\__  `-`  ___/-. /
         ___`. .'  /--.--\  `. . __
      ."" '<  `.___\_<|>_/___.'  >'"".
     | | :  `- \`.;`\ _ /`;.`/ - ` : | |
     \  \ `-.   \_ __\ /__ _/   .-` /  /
======`-.____`-.___\_____/___.-`____.-'======
                   `=---='
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
            佛祖保佑       永无BUG
 * 有点难QWQ，为了代码可读性做了一下代码格式化
 */

#include "catalog.h"
#include "demo.h"
#include "tests.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//默认控制台不知道怎么是GBK，要手动设置为UTF-8要不然有乱码，队友别动这块
#ifdef _WIN32
#include <windows.h>
#endif

/* 用来读取键盘输入的内容，目录可能有空格，所以不能用scanf，要用fgets
 * prompt是提示信息，buf是缓冲区，n是缓冲区大小
 * 无返回值
 */
static void read_line(const char *prompt, char *buf, int n) {
    size_t len;
    printf("%s", prompt);//打印提示词
    fflush(stdout);//刷新缓冲区
    if (!fgets(buf, n, stdin)) {
        buf[0] = '\0';//如果读取失败，则将buf设置为空字符串
        return;
    }
    len = strlen(buf);//获取buf的长度
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[--len] = '\0';//如果buf的最后一个字符是换行符或回车符，就把他去掉
    }
}

/* 用来读取键盘输入的选项，只能输入数字
 * 返回值是读取到的数字
 */
static int read_choice(void) {
    char buf[32];
    char *end = NULL;//用来存储读取到的数字的结束位置
    long v;//用来存储读取到的数字
    read_line("请选择：", buf, (int)sizeof(buf));
    if (buf[0] == '\0') {
        return -1;//buf为空
    }
    v = strtol(buf, &end, 10);
    if (end == buf) {
        return -1;//buf为空
    }
    return (int)v;//返回读取到的数字
}

//菜单
static void print_menu(void) {
    printf("\n======== 校园课程资料目录树与路径检索系统 ========\n");
    printf("1  新建文件夹          2  新建文件\n");
    printf("3  删除结点            4  修改（重命名 / 改大小）\n");
    printf("5  按路径查询          6  浏览目录（回车显示整棵森林）\n");
    printf("7  输出从根到目标的路径（顺序栈回溯）\n");
    printf("8  先序 / 中序 / 后序遍历（非递归）\n");
    printf("9  森林与二叉树互转，并验证可逆\n");
    printf("10 按文件名关键词查找（朴素模式匹配）\n");
    printf("11 按扩展名过滤\n");
    printf("12 同层文件：顺序表/链表、查找与排序对照\n");
    printf("13 载入示例森林        14 运行边界测试\n");
    printf("0  退出\n");
}

//创建文件夹
static void do_create_dir(Forest *f) {
    char path[MAX_PATH];//用来存储文件夹路径
    int rc;
    read_line("文件夹路径：", path, MAX_PATH);
    rc = catalog_mkdir(f, path);//创建文件夹
    printf("%s\n", rc == CAT_OK ? "已创建文件夹。" : catalog_err(rc));
}

//创建文件
static void do_create_file(Forest *f) {
    char path[MAX_PATH];//用来存储文件路径
    char buf[64];
    char *end = NULL;
    long size;
    int rc;
    read_line("文件路径：", path, MAX_PATH);
    read_line("文件大小：", buf, (int)sizeof(buf));
    size = strtol(buf, &end, 10);
    if (end == buf || size < 0) {
        printf("%s\n", catalog_err(CAT_INVALID));
        return;
    }
    rc = catalog_touch(f, path, size);
    printf("%s\n", rc == CAT_OK ? "已创建文件。" : catalog_err(rc));
}

static void do_delete(Forest *f) {
    char path[MAX_PATH];
    int rc;
    read_line("要删除的路径：", path, MAX_PATH);
    rc = catalog_delete(f, path);
    printf("%s\n", rc == CAT_OK ? "已删除（目录会连同子孙一起删除）。" : catalog_err(rc));
}

static void do_modify(Forest *f) {
    char path[MAX_PATH];
    char extra[MAX_NAME];
    int which, rc;
    read_line("要修改的路径：", path, MAX_PATH);
    printf("1 重命名    2 修改文件大小\n");
    which = read_choice();
    if (which == 1) {
        read_line("新名称（不能含 / ，且不能与同层重名）：", extra, MAX_NAME);
        rc = catalog_rename(f, path, extra);
        printf("%s\n", rc == CAT_OK ? "已重命名。" : catalog_err(rc));
    } else if (which == 2) {
        char *end = NULL;
        long size;
        read_line("新的大小：", extra, MAX_NAME);
        size = strtol(extra, &end, 10);
        if (end == extra || size < 0) {
            printf("%s\n", catalog_err(CAT_INVALID));
            return;
        }
        rc = catalog_set_size(f, path, size);
        printf("%s\n", rc == CAT_OK ? "已修改大小。" : catalog_err(rc));
    } else {
        printf("无效选项。\n");
    }
}

/*
 * 同层实验。回车时优先用示例里的“第3章”，那里有两个文件，查找和排序看得到结果。
 * 路径不存在或指向文件时，仍然运行与目录无关的算法对照，避免答辩时空白。
 */
static void do_level_demo(Forest *f) {
    char path[MAX_PATH];
    CSNode *dir = NULL;
    SeqList files;
    read_line("同层目录路径（直接回车则使用“计算机学院/数据结构/第3章”，若不存在就只做独立实验）：", path, MAX_PATH);
    if (path[0] == '\0') {
        dir = catalog_find(f, "计算机学院/数据结构/第3章");
    } else {
        dir = catalog_find(f, path);
        if (!dir) {
            printf("路径不存在，下面只做与当前目录无关的对照实验。\n");
        } else if (dir->kind != NK_DIR) {
            printf("该路径是文件，不是目录。下面只做独立对照实验。\n");
            dir = NULL;
        }
    }
    seq_init(&files);
    if (dir) {
        catalog_collect_level(dir, &files, NULL);
    }
    demo_all_algorithms(dir ? &files : NULL);
    seq_destroy(&files);
}

static void run_menu(Forest *f) {
    for (;;) {
        int choice;
        char path[MAX_PATH];
        print_menu();
        choice = read_choice();
        if (choice == 0) {
            printf("已退出。\n");
            return;
        }
        switch (choice) {
        case 1:
            do_create_dir(f);
            break;
        case 2:
            do_create_file(f);
            break;
        case 3:
            do_delete(f);
            break;
        case 4:
            do_modify(f);
            break;
        case 5:
            read_line("查询路径：", path, MAX_PATH);
            catalog_show_node(f, path);
            break;
        case 6:
            read_line("目录路径（直接回车显示整棵森林）：", path, MAX_PATH);
            catalog_show_dir(f, path);
            break;
        case 7: {
            CSNode *n;
            read_line("目标路径：", path, MAX_PATH);
            n = catalog_find(f, path);
            if (!n) {
                printf("%s\n", path[0] ? catalog_err(CAT_NOT_FOUND) : catalog_err(CAT_INVALID));
            } else {
                catalog_show_path(n);
            }
            break;
        }
        case 8:
            catalog_show_traversals(f);
            break;
        case 9:
            catalog_show_mapping_example();
            printf("\n======== 当前森林的转换 ========\n");
            catalog_show_conversion(f);
            break;
        case 10:
            read_line("文件名关键词：", path, MAX_PATH);
            catalog_show_keyword(f, path);
            break;
        case 11:
            read_line("扩展名（不含点，区分大小写；直接回车表示没有扩展名的文件）：", path, MAX_PATH);
            catalog_show_ext(f, path);
            break;
        case 12:
            do_level_demo(f);
            break;
        case 13: {
            int rc = catalog_load_sample(f);
            printf("%s\n", rc == CAT_OK ? "已载入示例：两棵课程树、空目录、重名文件。" : catalog_err(rc));
            if (rc == CAT_OK) {
                catalog_print_forest(f);
            }
            break;
        }
        case 14:
            run_all_tests();
            break;
        default:
            printf("无效选项。\n");
            break;
        }
    }
}

/*
 * 参数 test 只跑自测，不进入菜单，便于编译后立刻检查边界用例。
 * Windows 下把控制台改成 UTF-8，中文路径和菜单才能按源文件里的编码显示。
 */
int main(int argc, char **argv) {
    Forest forest;
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif
    forest_init(&forest);
    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        int failed = run_all_tests();
        forest_destroy(&forest);
        return failed == 0 ? 0 : 1;
    }
    printf("数据结构只使用手写的线性表、栈、串、孩子兄弟树和二叉树。\n");
    printf("建议先选 13 载入示例，再用 8、9、10、12 看遍历、转换和查找排序。\n");
    run_menu(&forest);
    forest_destroy(&forest);
    return 0;
}
