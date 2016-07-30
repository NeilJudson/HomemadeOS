/* 告诉C编译器，有一个函数在别的文件里 */

void io_hlt(void);

/* 函数声明不用{}，而用;，这表示函数是在别的文件里 */

void HariMain(void)
{

fin:
	io_hlt(); /* 执行naskfunc.nas里的_io_hlt */
	goto fin;

}
