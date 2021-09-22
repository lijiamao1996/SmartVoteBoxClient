#include"head.h" 
#define BUFF_SIZE 1024
#define MAXLINE 80
#define SER_PORT 12345
#define FILE_NAME_SIZE 512
void scanner_function ();
void control_function (void *ptr);
void send_function (void *ptr);
void scanner_main();
static void set_widget_font_size(GtkWidget *widget, int size, gboolean is_button);

GtkWidget *window;
GtkWidget *box;
GtkWidget *fixed;
GtkWidget *button_start;
GtkWidget *button_pause;
GtkWidget *button_exit;
GtkWidget *button_mode;
GtkWidget *button_restart;
GdkColor color;
GtkWidget *label_one;
int sc_main_is_running = 0;
int querywidget_isrunning = 0;
int auto_feed_mode = 1;
int global_isrunning;
GThread *ScanMain;
pthread_mutex_t mutex,mutex_global_isrunning;

char filename[1024][256];
char foldername[128];
char curpath[64];
char *devicename;
int len = 0;
int tag = 0;
void change_1();
void change_2();
void split(char *src,const char *separator,char **dest,int *num) {
	/*
		src 源字符串的首地址(buf的地址) 
		separator 指定的分割字符
		dest 接收子字符串的数组
		num 分割后子字符串的个数
	*/
     char *pNext;
     int count = 0;
     if (src == NULL || strlen(src) == 0) //如果传入的地址为空或长度为0，直接终止 
        return;
     if (separator == NULL || strlen(separator) == 0) //如未指定分割的字符串，直接终止 
        return;
     pNext = (char *)strtok(src,separator); //必须使用(char *)进行强制类型转换(虽然不写有的编译器中不会出现指针错误)
     while(pNext != NULL) {
          *dest++ = pNext;
          ++count;
         pNext = (char *)strtok(NULL,separator);  //必须使用(char *)进行强制类型转换
    }  
    *num = count;
} 	

int cmd(char* cmd, char* result)
{
    char buffer[10240];
    FILE* pipe = popen(cmd, "r");
    if (!pipe)
    return -1;
    while(!feof(pipe)) {
        if(fgets(buffer, 4096, pipe)){
            strcat(result, buffer);
        }
    }
    pclose(pipe);
    return 0;
}

char* genRandomString(int length)
{
	int flag, i;
	char* string;
	srand((unsigned) time(NULL ));
	if ((string = (char*) malloc(length)) == NULL )
	{
		printf("Malloc failed!\n");
		return NULL ;
	}
 
	for (i = 0; i < length; i++)
	{
		flag = rand() % 3;
		switch (flag)
		{
			case 0:
				string[i] = 'A' + rand() % 26;
				break;
			case 1:
				string[i] = 'a' + rand() % 26;
				break;
			case 2:
				string[i] = '0' + rand() % 10;
				break;
			default:
				string[i] = 'x';
				break;
		}
	}
	string[length] = '\0';
	return string;
}

int timesort (const struct dirent **__e1, const struct dirent **__e2){
	struct stat statbuf1,statbuf2;
	//whether foldername+d_name or not
	char *total1 = (char *)malloc(strlen(foldername)+strlen((*__e1)->d_name));
	char *total2 = (char *)malloc(strlen(foldername)+strlen((*__e2)->d_name));
	sprintf(total1,"%s/%s",foldername,(*__e1)->d_name);
	sprintf(total2,"%s/%s",foldername,(*__e2)->d_name);
	stat(total1, &statbuf1);
	stat(total2, &statbuf2);
	//first sort by create time, then sort by name
	//printf("total1=%s,total2=%s\n",total1,total2);
	free(total1);
	free(total2);
	if(statbuf1.st_ctime != statbuf2.st_ctime) return statbuf1.st_ctime-statbuf2.st_ctime;
	else return strcmp((*__e1)->d_name,(*__e2)->d_name);
}

int file_count(char* path,int flag)
{
	pthread_mutex_lock(&mutex);
    struct dirent **namelist;
	int n;
	int i=0;
	len = 0;
	n = scandir(path, &namelist, 0, timesort);
	if (n < 0)	
	{
		perror("not found\n");
	}
	else
	{
		while(i<n)
		{
			if(strncmp(namelist[i]->d_name, ".", 1) != 0){
				//printf("%s\n", namelist[i]->d_name);
				if(flag==1) len++;
				if(flag==2) sprintf(filename[len++],"%s/%s",foldername,namelist[i]->d_name);
			}
			//free(namelist[i]);
			i++;
		}
		//free(namelist);
	}
	pthread_mutex_unlock(&mutex);
    if(flag==1) return len;
    else return 0;
}

int get_file_size(FILE * file_handle)
{
	//获取当前读取文件的位置 进行保存
	unsigned int current_read_position=ftell( file_handle );
	int file_size;
	fseek( file_handle,0,SEEK_END );
	//获取文件的大小
	file_size=ftell( file_handle );
	//恢复文件原来读取的位置
	fseek( file_handle,current_read_position,SEEK_SET );

	return file_size;
}


void destroy(GtkWidget *widget,gpointer data)
{
    gtk_main_quit();
}

void error_quit(char* msg){
	
	char setting[128];
	sprintf(setting,"<span foreground='red' font-weight='bold' font_desc='60'>%s</span>",msg);
	gdk_threads_enter();  
	gtk_label_set_markup(GTK_LABEL(label_one), setting); 
	gdk_color_parse ("yellow", &color);
	//gtk_widget_modify_bg (GTK_WIDGET(window), GTK_STATE_NORMAL, &color);//背景色
	gtk_widget_set_sensitive(button_mode, FALSE);
	gtk_widget_set_sensitive(button_restart, TRUE);
	gdk_threads_leave();  
	
	//sleep(5);
	//destroy(window,NULL);
}

void btn_restart_program(GtkWidget *widget,gpointer data){
	
	destroy(window,NULL);
	//sleep(1);
	char* command = "./gtk";
	char bufferr[80];
	FILE* fp = popen(command, "r");
	fgets(bufferr,sizeof(bufferr),fp);
	printf("%s",bufferr);
	pclose(fp);
    
	
}

void btn1_run_scmain(GtkWidget *widget,gpointer data){
	if(sc_main_is_running==0) {
		memset(filename,0,sizeof(filename));
		len = 0;
		tag = 0;
		ScanMain = g_thread_new("scanner_main",(GThreadFunc)scanner_main,NULL);
		sc_main_is_running = 1;
		g_print("后台主线程开始\n");
		gtk_label_set_markup(GTK_LABEL(label_one),
		"<span foreground='black' font-weight='bold' font_desc='64'>投票箱已启动\n 正在连接······</span>");
	}
}

void btn2_pause_scmain(GtkWidget *widget,gpointer data){
	if(sc_main_is_running==1) {
		//g_thread_exit(ScanMain);
		pthread_cancel((pthread_t)ScanMain);
		sc_main_is_running = 0;
		g_print("后台主线程停止\n");
		gtk_label_set_markup(GTK_LABEL(label_one),
		"<span foreground='black' font-weight='bold' font_desc='64'>投票箱已停止\n 按开始启动</span>");
	}
}

void btn_change_mode(GtkWidget *widget,gpointer data){
	if(auto_feed_mode == 1){
		gtk_button_set_label(GTK_BUTTON(button_mode),"手动");
		set_widget_font_size(button_mode,30,TRUE);
		auto_feed_mode = 0;
		g_print("auto_feed_mode=%d,now it's manual.\n",auto_feed_mode);
	}
	else if(auto_feed_mode == 0){
		gtk_button_set_label(GTK_BUTTON(button_mode),"自动");
		set_widget_font_size(button_mode,30,TRUE);
		auto_feed_mode = 1;
		g_print("auto_feed_mode=%d,now it's auto.\n",auto_feed_mode);
	}
}

void button_continuelast_func(GtkWidget *widget,gpointer data){
	FILE *last=fopen("lastfolder","r");
    char *lastfoldername;
    fseek(last,0,SEEK_END);
	long lSize = ftell(last);
	lastfoldername=(char*)malloc(lSize+1);
	rewind(last); 
	fread(lastfoldername,sizeof(char),lSize,last);
	lastfoldername[lSize] = '\0';
	printf("%s\n",lastfoldername);
	
	sprintf(foldername,"%s",lastfoldername);
	//foldername = lastfoldername;
	printf("lastfoldername = %s\n",lastfoldername);
	free(lastfoldername);
	querywidget_isrunning = 1;
	gtk_widget_destroy((GtkWidget*) data);
	printf("%s\n",foldername);
	printf("continue last btn-------closed");
}

void button_newfolder_func(GtkWidget *widget,gpointer data){
	
	querywidget_isrunning = 2;
	gtk_widget_destroy((GtkWidget*) data);
	printf("new folder btn-------closed");
}

void button_cancel_func(GtkWidget *widget,gpointer data){
	gtk_widget_destroy((GtkWidget*) data);
	printf("cancel quit button  --clicked.");
}

static void set_widget_font_size(GtkWidget *widget, int size, gboolean is_button)
{
	GtkWidget *labelChild;  
	PangoFontDescription *font;  
	gint fontSize = size;  
 
	font = pango_font_description_from_string("Sans");          //"Sans"字体名   
	pango_font_description_set_size(font, fontSize*PANGO_SCALE);//设置字体大小   
 
	if(is_button){
		labelChild = gtk_bin_get_child(GTK_BIN(widget));//取出GtkButton里的label  
	}else{
		labelChild = widget;
	}
 
	//设置label的字体，这样这个GtkButton上面显示的字体就变了
	gtk_widget_modify_font(GTK_WIDGET(labelChild), font);
	pango_font_description_free(font);
}

void chang_background(GtkWidget *widget, int w, int h, const gchar *path)
{
	gtk_widget_set_app_paintable(widget, TRUE);		//允许窗口可以绘图
	gtk_widget_realize(widget);	
 
	/* 更改背景图时，图片会重叠
	 * 这时要手动调用下面的函数，让窗口绘图区域失效，产生窗口重绘制事件（即 expose 事件）。
	 */
	gtk_widget_queue_draw(widget);
 
	GdkPixbuf *src_pixbuf = gdk_pixbuf_new_from_file(path, NULL);	// 创建图片资源对象
	// w, h是指定图片的宽度和高度
	GdkPixbuf *dst_pixbuf = gdk_pixbuf_scale_simple(src_pixbuf, w, h, GDK_INTERP_BILINEAR);
 
	GdkPixmap *pixmap = NULL;
 
	/* 创建pixmap图像; 
	 * NULL：不需要蒙版; 
	 * 123： 0~255，透明到不透明
	 */
	gdk_pixbuf_render_pixmap_and_mask(dst_pixbuf, &pixmap, NULL, 128);
	// 通过pixmap给widget设置一张背景图，最后一个参数必须为: FASLE
	gdk_window_set_back_pixmap(widget->window, pixmap, FALSE);
 
	// 释放资源
	g_object_unref(src_pixbuf);
	g_object_unref(dst_pixbuf);
	g_object_unref(pixmap);
}

GtkWidget *create_test_window(){
	GtkWidget *label_query = gtk_label_new("label query");
    
    //GtkWidget *query=gtk_window_new (GTK_WINDOW_TOPLEVEL); //GTK_WINDOW_TOPLEVEL 是窗口 popup是最顶端 不可拖动
    GtkWidget *query=gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_set_position(GTK_WINDOW(query), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(query), 700, 400);
    
    gtk_window_set_title(GTK_WINDOW(query), "Notification");
    gdk_color_parse ("black", &color);
    
    GtkWidget *box_query = gtk_hbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(query),box_query);
    
    GtkWidget *fixed_query = gtk_fixed_new();
	gtk_widget_set_usize(fixed_query,600,300);
	gtk_box_pack_start(GTK_BOX(box_query),fixed_query,TRUE,TRUE,0);
	GtkWidget *button_continuelast = gtk_button_new_with_label("继续上次");
	gtk_fixed_put(GTK_FIXED(fixed_query),button_continuelast,100,200);
	GtkWidget *button_newfolder = gtk_button_new_with_label("新选举");
	gtk_fixed_put(GTK_FIXED(fixed_query),button_newfolder,400,200);
	gtk_fixed_put(GTK_FIXED(fixed_query),label_query,120,50);
	set_widget_font_size(button_continuelast,20,TRUE);
	set_widget_font_size(button_newfolder,20,TRUE);
    g_signal_connect (G_OBJECT (button_continuelast), "clicked",G_CALLBACK (button_continuelast_func), query);
    g_signal_connect (G_OBJECT (button_newfolder), "clicked",G_CALLBACK (button_newfolder_func), query);
	
    gtk_widget_modify_bg (GTK_WIDGET(query), GTK_STATE_NORMAL, &color);//背景色
    gtk_label_set_markup(GTK_LABEL(label_query),
	"<span foreground='white' font_desc='40'>是否继续上一次投票</span>");
	gtk_widget_set_usize(button_continuelast,130,70);
    gtk_widget_set_usize(button_newfolder,130,70);
    gtk_window_set_resizable(GTK_WINDOW(query),FALSE);
    
    return query;
}

GtkWidget *create_exit_window(){
	GtkWidget *label_query = gtk_label_new("label query");
    
    //GtkWidget *query=gtk_window_new (GTK_WINDOW_TOPLEVEL); //GTK_WINDOW_TOPLEVEL 是窗口 popup是最顶端 不可拖动
    GtkWidget *query=gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_set_position(GTK_WINDOW(query), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(query), 700, 400);
    
    gtk_window_set_title(GTK_WINDOW(query), "Notification");
    gdk_color_parse ("black", &color);
    
    GtkWidget *box_query = gtk_hbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(query),box_query);
    
    GtkWidget *fixed_query = gtk_fixed_new();
	gtk_widget_set_usize(fixed_query,600,300);
	gtk_box_pack_start(GTK_BOX(box_query),fixed_query,TRUE,TRUE,0);
	GtkWidget *button_quit = gtk_button_new_with_label("确认");
	gtk_fixed_put(GTK_FIXED(fixed_query),button_quit,100,200);
	GtkWidget *button_cancel = gtk_button_new_with_label("取消");
	gtk_fixed_put(GTK_FIXED(fixed_query),button_cancel,400,200);
	gtk_fixed_put(GTK_FIXED(fixed_query),label_query,120,50);
	set_widget_font_size(button_quit,20,TRUE);
	set_widget_font_size(button_cancel,20,TRUE);
    g_signal_connect (G_OBJECT (button_quit), "clicked",G_CALLBACK (destroy), NULL);
    g_signal_connect (G_OBJECT (button_cancel), "clicked",G_CALLBACK (button_cancel_func), query);
	
    gtk_widget_modify_bg (GTK_WIDGET(query), GTK_STATE_NORMAL, &color);//背景色
    gtk_label_set_markup(GTK_LABEL(label_query),
	"<span foreground='white' font_desc='40'>确认退出吗？</span>");
	gtk_widget_set_usize(button_quit,130,70);
    gtk_widget_set_usize(button_cancel,130,70);
    gtk_window_set_resizable(GTK_WINDOW(query),FALSE);
    
    return query;
}


void button_exit_func(){
	
	GtkWidget *exit_notification = create_exit_window();
    gtk_widget_show_all (exit_notification); 
}

GtkWidget *create_main_window(){
	label_one = gtk_label_new("label one");
    
    GdkScreen* screen;
    gint width, height;
    screen = gdk_screen_get_default();
    width = gdk_screen_get_width(screen);
    height = gdk_screen_get_height(screen);
    printf("screen width: %d, height: %d\n", width, height);
    
    //window=gtk_window_new (GTK_WINDOW_TOPLEVEL); //GTK_WINDOW_TOPLEVEL 是窗口 popup是最顶端 不可拖动
    window=gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
    //gtk_window_set_decorated(GTK_WINDOW(window), FALSE); /* hide the title bar and the boder */
    
    gtk_window_set_title(GTK_WINDOW(window), "扫描仪客户端");
    gtk_signal_connect (GTK_OBJECT(window),"delete_event",GTK_SIGNAL_FUNC(destroy),NULL);
    gdk_color_parse ("black", &color);
    
    box = gtk_hbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(window),box);
    
    fixed = gtk_fixed_new();
	gtk_widget_set_usize(fixed,width,height);
	gtk_box_pack_start(GTK_BOX(box),fixed,TRUE,TRUE,0);
	//button_start = gtk_button_new_with_label("新投票");
	//gtk_fixed_put(GTK_FIXED(fixed),button_start,width/9,height*2/3);
	//button_pause = gtk_button_new_with_label("停止");
	//gtk_fixed_put(GTK_FIXED(fixed),button_pause,width/3,height*2/3);
	button_exit = gtk_button_new_with_label("退出");
	gtk_fixed_put(GTK_FIXED(fixed),button_exit,width*5/6,height*5/6);
	button_mode = gtk_button_new_with_label("自动");
	gtk_fixed_put(GTK_FIXED(fixed),button_mode,width*1/4,height*2/3);
	button_restart = gtk_button_new_with_label("刷新");
	gtk_fixed_put(GTK_FIXED(fixed),button_restart,width*2/3,height*2/3);
	
	gtk_fixed_put(GTK_FIXED(fixed),label_one,width/4,height/5);
	//set_widget_font_size(button_start,20,TRUE);
	//set_widget_font_size(button_pause,20,TRUE);
	set_widget_font_size(button_exit,25,TRUE);
	set_widget_font_size(button_mode,30,TRUE);
	set_widget_font_size(button_restart,30,TRUE);
    //g_signal_connect (G_OBJECT (button_start), "clicked",G_CALLBACK (btn1_run_scmain), NULL);
    //g_signal_connect (G_OBJECT (button_pause), "clicked",G_CALLBACK (btn2_pause_scmain), NULL);
    g_signal_connect (G_OBJECT (button_exit), "clicked",G_CALLBACK (button_exit_func), NULL);
    g_signal_connect (G_OBJECT (button_mode), "clicked",G_CALLBACK (btn_change_mode), NULL);
    g_signal_connect (G_OBJECT (button_restart), "clicked",G_CALLBACK (btn_restart_program), NULL);
    
	
    
    chang_background(window, width, height, "image.jpg");
    //gtk_widget_modify_bg (GTK_WIDGET(window), GTK_STATE_NORMAL, &color);//背景色
    gtk_label_set_markup(GTK_LABEL(label_one),
	"<span foreground='black' font-weight='bold' font_desc='64'>投票箱已启动\n正在连接······</span>");
	
    //gtk_widget_set_usize(window,960,550);
    //gtk_widget_set_usize(button_start,130,70);
    //gtk_widget_set_usize(button_pause,130,70);
    gtk_widget_set_usize(button_exit,100,70);
    gtk_widget_set_usize(button_mode,200,100);
    gtk_widget_set_usize(button_restart,200,100);
    gtk_widget_set_sensitive(button_restart, FALSE);
    gtk_window_set_resizable(GTK_WINDOW(window),FALSE);
    
    return window;
}

void scanner_main(){
	
    
	while(querywidget_isrunning == 0){
	}
	printf("--------------------------------stop----------------------------");
	if(querywidget_isrunning==1){
		printf("foldername=%s\n",foldername);
		file_count(foldername,2);
		tag = len;
	}
	else if(querywidget_isrunning==2){
		struct timeval tv;
		gettimeofday(&tv,NULL);//获取1970-1-1到现在的时间结果保存到tv中
		uint64_t sec=tv.tv_sec;
		struct tm cur_tm;//保存转换后的时间结果
		localtime_r((time_t*)&sec,&cur_tm);
		char * newdir=(char*)malloc(64);
		int offset = 1;
		snprintf(newdir,64,"%d%02d%02d-%04d",cur_tm.tm_year+1900,cur_tm.tm_mon+1,cur_tm.tm_mday,offset);
		
		char * prepath = "/home/mit/Public/";
		char * fdname = (char*) malloc(strlen(newdir)+strlen(prepath));
		strcpy(fdname,prepath);
		strcat(fdname,newdir);
		sprintf(foldername,"%s",fdname);
		//foldername = fdname;
		printf("foldername=%s\n",foldername);
		char * pre = "mkdir ";
		char * operate = (char*) malloc(strlen(fdname)+strlen(pre));
		strcpy(operate,pre);
		strcat(operate,fdname);
		int mkdir_result = system(operate);
		printf("make directory result : %d\n",mkdir_result);
		//if folder exists, offset+1, try to make a new folder
		while(mkdir_result!=0){
			offset++;
			snprintf(newdir,64,"%d%02d%02d-%04d",cur_tm.tm_year+1900,cur_tm.tm_mon+1,cur_tm.tm_mday,offset);
			strcpy(fdname,prepath);
			strcat(fdname,newdir);
			sprintf(foldername,"%s",fdname);
			//foldername = fdname;
			strcpy(operate,pre);
			strcat(operate,fdname);
			mkdir_result = system(operate);
			printf("make directory result : %d\n",mkdir_result);
		}
		printf("newdir=%s,operate=%s,fdname=%s\n",newdir,operate,fdname);
		free(newdir);
		free(operate);
		free(fdname);
		FILE *last=fopen("lastfolder","w");
		if(NULL == last)
		{
			printf("open last error!");
			return;
		}
		fprintf(last, "%s", foldername);
		fclose(last);
		
		len = 0;
		tag = 0;
	}
	
	
	memset(filename,0,sizeof(filename));
	
	
	char buffer[10240]="";
    char *revbuf[8] = {0};
    char *revbuf1[8] = {0};
    int nummm=0;
    while(cmd("scanimage -L", buffer) == 0){
		split(buffer,"'",revbuf,&nummm);
        split(revbuf[0],"`",revbuf1,&nummm);
        if(revbuf1[1]==NULL) {
			printf("Scanner is not connected.\n");
			gdk_threads_enter();  
			gtk_label_set_markup(GTK_LABEL(label_one),
			"<span foreground='black' font-weight='bold' font_desc='64'>投票箱未连接\n  请检查</span>"); 
			gdk_color_parse ("yellow", &color);
			//gtk_widget_modify_bg (GTK_WIDGET(window), GTK_STATE_NORMAL, &color);//背景色
			gdk_threads_leave();  
			sleep(10);
		}
        else {
			printf("cmd ps output : %s\n", revbuf1[1]);
			devicename=revbuf1[1];
			gdk_threads_enter();  
			gtk_label_set_markup(GTK_LABEL(label_one),
			"<span foreground='black' font-weight='bold' font_desc='64'>投票箱已连接\n等待软件连接······</span>"); 
			gdk_color_parse ("green", &color);
			//gtk_widget_modify_bg (GTK_WIDGET(window), GTK_STATE_NORMAL, &color);//背景色
			gdk_threads_leave();  
			break;
		}
	}
	
	struct sockaddr_in servaddr;
    char* serverIP=NULL;
 
    int sockfd;
    FILE *cfg=fopen("config1.cfg","r");
    int connected = 0;
    
    fseek(cfg,0,SEEK_END);
	long lSize = ftell(cfg);
	serverIP=(char*)malloc(lSize+1);
	rewind(cfg); 
	fread(serverIP,sizeof(char),lSize,cfg);
	serverIP[lSize-1] = '\0';
    
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    
    signal(SIGPIPE,SIG_IGN);
    //int nosig_value = 1;
	//setsockopt(sockfd, SOL_SOCKET, MSG_NOSIGNAL, &nosig_value, sizeof(nosig_value));
	
	//set send timeout & recv timeout
	struct timeval timeout={10,0};//3s
    int ret_set_sendtimeout=setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
    int ret_set_recvtimeout=setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
    printf("sendtimeout=%d, recvtimeout=%d.\n",ret_set_sendtimeout,ret_set_recvtimeout);

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET,serverIP,&servaddr.sin_addr);
    servaddr.sin_port = htons(SER_PORT);
    printf("serverip=%s\n",serverIP);
    free(serverIP);
	
	while(global_isrunning == 1){
		while(connected == 0){
			if(connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr))<0){
				printf("connet error:%s\n",strerror(errno));
				sleep(5);
			}
			else{//开一个线程
				connected = 1;
				
				pthread_t thread_control;
				int ret_thrd_ctrl;
				ret_thrd_ctrl = pthread_create(&thread_control, NULL, (void *)&control_function, (void *)(intptr_t) sockfd);

				// 线程创建成功，返回0,失败返回失败号
				if (ret_thrd_ctrl != 0) {
					printf("控制线程创建失败\n");
				} else {
					printf("控制线程创建成功\n");
					
					gdk_threads_enter();  
					gtk_label_set_markup(GTK_LABEL(label_one),
					"<span foreground='black' font-weight='bold' font_desc='64'>软件连接成功\n等待投票······</span>"); 
					gdk_color_parse ("green", &color);
					//gtk_widget_modify_bg (GTK_WIDGET(window), GTK_STATE_NORMAL, &color);//背景色
					gdk_threads_leave();  
					
				}
			}
		}
	}
	printf("scanner_main is over.\n");
}

void change_1(){
	gdk_threads_enter();  
	gtk_label_set_markup(GTK_LABEL(label_one),
				"<span foreground='black' font-weight='bold' font_desc='64'> 正在重传······\n</span>"); 
	gdk_color_parse ("green", &color);
				//gtk_widget_modify_bg (GTK_WIDGET(window), GTK_STATE_NORMAL, &color);//背景色
	gdk_threads_leave();  
}
void change_2(){
	gdk_threads_enter();  
				gtk_label_set_markup(GTK_LABEL(label_one),
				"<span foreground='black' font-weight='bold' font_desc='64'>重传已完成\n</span>"); 
				gdk_color_parse ("green", &color);
				//gtk_widget_modify_bg (GTK_WIDGET(window), GTK_STATE_NORMAL, &color);//背景色
				gdk_threads_leave(); 
}


void control_function( void *ptr ){
	int sockfd = (intptr_t)(int*)ptr;
    char bufc[BUFF_SIZE];
    pthread_t thread_scanner;
    pthread_t thread_send;
    pthread_mutex_init (&mutex, NULL);
    int isExist = 0;
    while(global_isrunning == 1){
		memset(bufc,0,BUFF_SIZE);
		int ret = recv(sockfd,bufc,BUFF_SIZE,0);
		printf("ret=%d.\n",ret);
		if(ret==0 ||ret == -1){
			pthread_mutex_lock(&mutex_global_isrunning);
			global_isrunning = 0;
			pthread_mutex_unlock(&mutex_global_isrunning);
			break;
		}
		printf("%s\n",bufc);
        if(strcmp(bufc,"helloclient")==0){ 
			//printf("无操作\n");
		}
        else if(strcmp(bufc,"start")==0){
			if(isExist==0){
				int ret_thrd_scnr;
				int ret_thrd_send;
				ret_thrd_scnr = pthread_create(&thread_scanner, NULL, (void *)&scanner_function, NULL);
				ret_thrd_send = pthread_create(&thread_send, NULL, (void *)&send_function, (void *) ptr);
				
				// 线程创建成功，返回0,失败返回失败号
				if (ret_thrd_scnr == 0 && ret_thrd_send == 0) {
					printf("扫描发送线程创建成功\n");
					
					gdk_threads_enter();  
					gtk_label_set_markup(GTK_LABEL(label_one),
					"<span foreground='black' font-weight='bold' font_desc='64'>读票中······</span>"); 
					gdk_color_parse ("green", &color);
					//gtk_widget_modify_bg (GTK_WIDGET(window), GTK_STATE_NORMAL, &color);//背景色
					gdk_threads_leave();  
					
					isExist = 1;
				} 
				else { printf("扫描发送线程\n创建失败\n"); }
			}
		}
		else if(strcmp(bufc,"shutdown")==0){
			if(isExist == 1) {
				int ret_cancel_scnr;
				int ret_cancel_send;
				ret_cancel_scnr = pthread_cancel(thread_scanner);
				ret_cancel_send = pthread_cancel(thread_send);
				if(ret_cancel_scnr == 0 && ret_cancel_send == 0) {
					printf("扫描发送线程\n终止成功\n");
					
					gdk_threads_enter();  
					gtk_label_set_markup(GTK_LABEL(label_one),
					"<span foreground='black' font-weight='bold' font_desc='64'>  已停止投票</span>"); 
					gdk_color_parse ("red", &color);
					//gtk_widget_modify_bg (GTK_WIDGET(window), GTK_STATE_NORMAL, &color);//背景色
					gdk_threads_leave();  
					
					isExist = 0;
				} 
				else { printf("扫描发送线程终止失败\n");}
			}
		}
		else if(strcmp(bufc,"turnoff")==0){
			system("poweroff");
		}
		else if(strcmp(bufc,"reboot")==0){
			system("reboot");
		}
		else if(strcmp(bufc,"collate")==0){
			//printf("enterwhile(1)\n");
			file_count(foldername,2);
			printf("there are %d files\n",len);
			
			char buf_collate[256];
			sprintf(buf_collate,"%s@%d",foldername,len);
			printf("buf_colate=%s, strlen=%ld",buf_collate,strlen(buf_collate));
			if(send(sockfd,buf_collate,strlen(buf_collate),0) == -1)// cannot be sent ,whatever it is
				printf("send buf_collate error.\n");
			
			printf("collate is done.\n");
			//发送完文件列表后阻塞接收结果，是否需要重传
			memset(bufc,0,BUFF_SIZE);
			recv(sockfd,bufc,BUFF_SIZE,0);
			printf("%s\n",bufc);
			if(strcmp(bufc,"verified")==0){
				printf("验证成功，无漏传\n");
				
				gdk_threads_enter();  
				gtk_label_set_markup(GTK_LABEL(label_one),
				"<span foreground='black' font-weight='bold' font_desc='64'>  验 证 成 功\n  无 漏 传</span>"); 
				gdk_color_parse ("green", &color);
				//gtk_widget_modify_bg (GTK_WIDGET(window), GTK_STATE_NORMAL, &color);//背景色
				gdk_threads_leave();  
				
			}
			else if(strncmp(bufc,"resend",6)==0){   // resend 
				/*
				gdk_threads_enter();  
				gtk_label_set_markup(GTK_LABEL(label_one),
				"<span foreground='black' font-weight='bold' font_desc='64'> 正在重传······\n</span>"); 
				gdk_color_parse ("green", &color);
				//gtk_widget_modify_bg (GTK_WIDGET(window), GTK_STATE_NORMAL, &color);//背景色
				gdk_threads_leave();  
				*/
				change_1();
				char resend_start[16];
				strncpy(resend_start,bufc+6,strlen(bufc)-6);
				int resend_start_index = atoi(resend_start)*2;
				
				//while(1){}
				char buf_resend[BUFF_SIZE];
				file_count(foldername,2);
				while(len!=0 && resend_start_index<len){
					//for(int i=0;i<len;i++) printf("%s\n",filename[i]);
					printf("len=%d,tag_resend=%d\n",len,resend_start_index);
					
					char head[256];
					printf("filename[resend_start_index]=%s\n",filename[resend_start_index]);
					FILE *fp = fopen(filename[resend_start_index],"rb");
					int fpsize = get_file_size(fp);
					sprintf(head,"%s@%d",filename[resend_start_index],fpsize); //在文件名和文件大小中间加一个@作为分割符，方便
					printf("head=%s\n",head);
					if(send(sockfd,head,strlen(head),0)==-1)
						printf("send filelength error");
					if(NULL==fp)
						printf("fopen error");
					else
					{
						memset(buf_resend,0,BUFF_SIZE);
						int length=0;
						while((length=fread(buf_resend,sizeof(char),BUFF_SIZE,fp))>0)
						{
							if(send(sockfd,buf_resend,length,0)==-1)
								strerr("send error");
							memset(buf_resend,0,BUFF_SIZE);
						}
						printf("FILE %s transfer successful\n",filename[resend_start_index]);
					}
					fclose(fp);
					resend_start_index++;
					
				}
				
				char resend_finish[64];
				tag=resend_start_index;
				strcpy(resend_finish,"resend_finished@");
				if(send(sockfd,resend_finish,strlen(resend_finish),0) == -1)  // cannot be sent ,whatever it is
					printf("send resend_finish error");
				change_2();
				/*
				gdk_threads_enter();  
				gtk_label_set_markup(GTK_LABEL(label_one),
				"<span foreground='black' font-weight='bold' font_desc='64'>重传已完成\n</span>"); 
				gdk_color_parse ("green", &color);
				//gtk_widget_modify_bg (GTK_WIDGET(window), GTK_STATE_NORMAL, &color);//背景色
				gdk_threads_leave();  
				*/
			}
			else{
				gdk_threads_enter();  
				gtk_label_set_markup(GTK_LABEL(label_one),
				"<span foreground='black' font-weight='bold' font_desc='64'>  既不是验证成功\n  也不是重传</span>"); 
				gdk_color_parse ("green", &color);
				gdk_threads_leave();  
			}
		}
		sleep(1);
	}
	// lose connection, forexample: server has been closed.
	// show"lose connection" and close the program after 5 secs.
	printf("control_function is over.\n");
	int ret_cancel_scnr;
	int ret_cancel_send;
	ret_cancel_scnr = pthread_cancel(thread_scanner);
	ret_cancel_send = pthread_cancel(thread_send);
	if(ret_cancel_scnr == 0 && ret_cancel_send == 0) {
		printf("扫描发送线程\n终止成功\n");
		isExist = 0;
	} 
	else { printf("扫描发送线程终止失败\n");}
	
	pthread_mutex_destroy(&mutex);
	error_quit("失 去 连 接\n请检查后刷新");
}

void scanner_function() {
	int abnormal = 0;
	
	
	gdk_threads_enter();  
	gtk_label_set_markup(GTK_LABEL(label_one),
	"<span foreground='black' font-weight='bold' font_desc='64'> 读票中······</span>"); 
	gdk_color_parse ("green", &color);
	//gtk_widget_modify_bg (GTK_WIDGET(window), GTK_STATE_NORMAL, &color);//背景色
	gdk_threads_leave();  
	
	
	
	while(global_isrunning == 1 && abnormal<2){
		int picnum = file_count(foldername,1);
		char* numstr=(char*) malloc(sizeof(char)*64);
		sprintf(numstr,"%d",picnum+1);
		char* sc1=(char*) malloc(sizeof(char)*1024);
		char* automode = (char*) malloc(sizeof(char)*8);
		if(auto_feed_mode == 0) sprintf(automode,"no");
		else if(auto_feed_mode == 1) sprintf(automode,"yes");
		sprintf(sc1,"scanimage -d %s --source ADF-duplex --brightness 127 --resolution 200 \
			--contrast 127 --jpeg-quality 7 --blank-page-skip=no --format=jpeg \
			--autofeed=%s --batch=",devicename, automode);
		char* sc2 = "/out%06d.jpg --batch-start=";
		char* sc_cmd = (char*) malloc(strlen(sc1)+strlen(sc2)+strlen(foldername)+strlen(numstr));
		//printf("sc_cmd malloc = %ld\n",strlen(sc_cmd));
		strcpy(sc_cmd,sc1);
		strcat(sc_cmd,foldername);
		strcat(sc_cmd,sc2);
		strcat(sc_cmd,numstr);
		//printf("numstr=%s,sc1=%s,automode=%s\n",numstr,sc1,automode);
		free(numstr);
		free(sc1);
		free(automode);
		//printf("%s\n",sc_cmd);
		
		int n=system(sc_cmd);
		//printf("sc_cmd=%s\n",sc_cmd);
		free(sc_cmd);
		printf("n-cmd:%d\n",n);
		if(n==1792) {
			abnormal = 0; //adf is empty
			//usleep(0.5*1000000);
			sleep(1);
		}
		if(n==0) {
			abnormal = 0; //scanning
			sleep(1);
		}
		else if(n==512 || n==256){  //scanner is not connected
			abnormal++;
			if(abnormal>=2) break;
		}
	}
	if(abnormal >= 2) error_quit("投票箱连接异常\n 请检查后重启");
	printf("scanner_function is over.\n");
}

void send_function( void *ptr ) {
	int sockfd = (intptr_t)(int*)ptr;
    char buf[BUFF_SIZE];
    
    while(global_isrunning == 1){
		file_count(foldername,2);
		//for(int i=0;i<len;i++) printf("%s\n",filename[i]);
		printf("len=%d,tag=%d\n",len,tag);
		
		if(len != 0 && tag<len){
			char head[256];
			printf("filename[tag]=%s\n",filename[tag]);
			FILE *fp = fopen(filename[tag],"rb");
			int fpsize = get_file_size(fp);
			sprintf(head,"%s@%d",filename[tag],fpsize); //在文件名和文件大小中间加一个@作为分割符，方便
			printf("head=%s\n",head);
			if(send(sockfd,head,strlen(head),0)==-1){
				//printf("send filelength error");
				strerr("send filelength error");
				pthread_mutex_lock(&mutex_global_isrunning);
				global_isrunning = 0;
				pthread_mutex_unlock(&mutex_global_isrunning);
				break;
			}
			if(NULL==fp){
				strerr("fopen error");
				pthread_mutex_lock(&mutex_global_isrunning);
				global_isrunning = 0;
				pthread_mutex_unlock(&mutex_global_isrunning);
				break;
			}
			else
			{
				memset(buf,0,BUFF_SIZE);
				int length=0;
				while((length=fread(buf,sizeof(char),BUFF_SIZE,fp))>0)
				{
					if(send(sockfd,buf,length,0)==-1){
						strerr("send error");
						pthread_mutex_lock(&mutex_global_isrunning);
						global_isrunning = 0;
						pthread_mutex_unlock(&mutex_global_isrunning);
						break;
					}
					memset(buf,0,BUFF_SIZE);
				}
				if(global_isrunning == 0) break;
				printf("FILE %s transfer successful\n",filename[tag]);
			}
			fclose(fp);
			tag++;
		}
		else {
			sleep(4);
		}
		sleep(1);
	}
	printf("send_function is over.\n");
}

void resend_image(){
	
}

int main( int argc, char *argv[] )
{
	gtk_init(&argc, &argv);
    
	/*线程的初始化*/  
    //if(!g_thread_supported()) g_thread_init(NULL);  
    gdk_threads_init();  
    
    global_isrunning = 1;
    pthread_mutex_init (&mutex_global_isrunning, NULL);
    
    /*创建线程*/  
    ScanMain = g_thread_new("scanner_main",(GThreadFunc)scanner_main,NULL);
    sc_main_is_running = 1;
    g_print("后台主线程开始\n");
	
	window = create_main_window();
    gtk_widget_show_all (window);   /*显示一个窗口*/
    GtkWidget *son = create_test_window();
    gtk_widget_show_all (son); 
    
    gdk_threads_enter();  
    gtk_main ();  
    gdk_threads_leave();  
    
    //pthread_mutex_destroy (&mutex_global_isrunning);
    return 0;
}
