#include "uni_gui_node.h"
#include "screen.h"
#include "image_widget.h"
#include "button_widget.h"
#include "text_widget.h"
#include "slide_widget.h"
#include "rect_widget.h"
#include "box_widget.h"
#include "slideloop_widget.h"
#include "svg_widget.h"
#include "gif_widget.h"

#include "aircondision_widget.h"
#include "switch_widget.h"
#include "table_widget.h"

void image_test(void)
{
	mimage *main = create_image_widget("main",0,0,1280,720);

	main->opt->set_picture(main,"/out/test2.jpg");

	add_node_to_screen((mwidget*)main);
}

static int button_click_cbk(void *param)
{
	printf("%s %d\r\n",__func__,__LINE__);
	return 0;
}

void button_test(void)
{
	mbutton *main = create_button_widget("main",0,0,1280,720);

	main->opt->image_set(main,"res/home_bg_1.png","res/home_bg_1.png");
	//main->opt->register_buttonclick_cbk(main,button_click_cbk,NULL);

	add_node_to_screen((mwidget*)main);

}

static void text_test(void)
{
	mtext *main = 	create_text_widget("main",0,0,1280,720);
	main->opt->change_text(main,"Hello OVRIBO!");
	textStyle style = {
		.textSize = 40,
		.textColor = 0xff000000,
		.textBKColor = 0xffffff00,
		.textAlpha = 0xff,
		.textAlign = ALIGN_TO_CENTER,
	};

	main->opt->set_style(main,&style);

	add_node_to_screen((mwidget*)main);
}

static void slide_test(void)
{
	mslide *main = 	create_slide_widget("main",0,0,1280,720);

	main->opt->set_mv_dir(main,SLIDE_NODE_MOVE_DIR_X);


	int i;
	mimage *img;
	for(i = 0; i < 10; i++){
		if(i == 0){
			img = create_image_widget("img1",0,100,378,252);
		}else{
			img = create_image_widget("img1",378+100,0,378,252);
			img->location_type = GUI_LOCATION_TYPE_PREV_BROTHERS;
		}

		if(i%2 == 0){
			img->opt->set_picture(img,"/out/pic2.jpg");
		}else{
			img->opt->set_picture(img,"/out/test2.jpg");
		}

		gui_node_add_child_node_to_end((mwidget*)img,(mwidget*)main);
	}

	printf("%s %d %p\r\n",__func__,__LINE__,main->opt);
	add_node_to_screen((mwidget*)main);
}

static void rect_test(void)
{
	//mrect *main = create_rect_widget("main", 0,0,1280,720);
	//main->opt->setBgColor(main,0x0,0xff);
	
	printf("%s %d\r\n",__func__,__LINE__);
	mimage *main = create_image_widget("main",0,0,1280,720);
	printf("%s %d opt:%p class:%s %p\r\n",__func__,__LINE__,main->opt,main->opt->class_name,main->opt->set_picture);
	main->opt->set_picture(main,"/out/test2.jpg");
	printf("%s %d\r\n",__func__,__LINE__);

	mrect *m = create_rect_widget("rect1", 300,200,200,100);
	unsigned char alpha = 0x2f;
	//m->opt->setBgColor(m,0xDCDCDC,alpha);

	printf("%s %d 0x%02x\r\n",__func__,__LINE__,alpha);

	gui_node_add_child_node_to_end(m,main);


	add_node_to_screen((mwidget*)main);
}

static void box_test(void)
{
	mimage *main = create_image_widget("main",0,0,480,480);
	main->opt->set_picture(main,"res/home_bg_1.png");

	mbox *mb = create_box_widget("box1",100,100,200,100);
	gui_node_add_child_node_to_end(mb,main);

	mrect *m = create_rect_widget("rect1", 0,0,200,100);
	m->location_type = GUI_LOCATION_TYPE_PARENT;
	unsigned char alpha = 0x3f;
	//m->opt->setBgColor(m,0xDCDCDC,alpha);
	printf("%s %d 0x%02x\r\n",__func__,__LINE__,alpha);
	gui_node_add_child_node_to_end(m,mb);

	//mb->opt->set_rotate(mb, 60);
	mb->opt->set_scale(mb, m->w * 2 , m->h * 2);
	mb->opt->set_auto_rotate(mb, 1, 2);

	mbox *mbt = create_box_widget("box2",300,300,100,100);

	mbt->opt->set_scale(mbt, m->w , m->h);
	mbt->opt->set_auto_rotate(mbt, 1, 4);
	gui_node_add_child_node_to_end(mbt,main);

	mtext *mt = create_text_widget("text",0,0,100,100);
	mt->opt->change_text(mt,"OVRIBO");
	textStyle style = {
		.textSize = 20,
		.textColor = 0xffffff,
		.textAlpha = 0xff,
		.textAlign = ALIGN_TO_CENTER,
		.transparentBKFlag = 1,
	};

	mt->opt->set_style(mt,&style);	
	gui_node_add_child_node_to_end(mt,mbt);
	
	add_node_to_screen((mwidget*)main);
}

static void box_slide_test(void)
{
	mimage *main = create_image_widget("main",0,0,1280,720);
	main->opt->set_picture(main,"/out/test2.jpg");

	mslide *slide = create_slide_widget("slide",0,0,1280,720);
	slide->opt->set_mv_dir(slide,SLIDE_NODE_MOVE_DIR_X);
	gui_node_add_child_node_to_end(slide,main);

	mbox *mb = create_box_widget("box1",0,200,400,200);
	gui_node_add_child_node_to_end(mb,slide);

	mrect *m = create_rect_widget("rect1", 0,0,400,200);
	m->location_type = GUI_LOCATION_TYPE_PARENT;
	unsigned char alpha = 0xff;
	//m->opt->setBgColor(m,0xDCDCDC,alpha);
	printf("%s %d 0x%02x\r\n",__func__,__LINE__,alpha);
	gui_node_add_child_node_to_end(m,mb);

	int colors[] = {0x696969, 0x6495ED, 0xADFF2F, 0xFFD700, 0x8B8B00, 0xFFC1C1, 0xFF6A6A,0xEE7942,0xFF1493};

	int i;
	for(i = 0; i < 9; i++){
		char name[16];
		sprintf(name,"box%d",i+2);
		mb = create_box_widget(name,50+400,0,400,200);
		mb->location_type = GUI_LOCATION_TYPE_PREV_BROTHERS;
		gui_node_add_child_node_to_end(mb,slide);


		sprintf(name,"rect%d",i+2);
		m = create_rect_widget(name, 0,0,400,200);
		m->location_type = GUI_LOCATION_TYPE_PARENT;
		//m->opt->setBgColor(m,colors[i],alpha);
		gui_node_add_child_node_to_end(m,mb);

		sprintf(name,"text%d",i+2);
		mtext *mt = create_text_widget(name,50,0,100,100);
		mt->opt->change_text(mt,name);
		textStyle style = {
			.textSize = 40,
			.textColor = 0x000000,
			.textAlpha = 0xff,
			//.textBKColor = 0xffffff00,
			.textAlign = ALIGN_TO_CENTER,
			.transparentBKFlag = 1,
		};

		mt->opt->set_style(mt,&style);
		gui_node_add_child_node_to_end(mt,mb);

	}

	add_node_to_screen((mwidget*)main);
}

static void button_slide_test(void)
{
	mimage *main = create_image_widget("main",0,0,1280,720);
	main->opt->set_picture(main,"/out/test2.jpg");

	//main->opt->activate_event(main,"onClick");

	mrect *mo = create_rect_widget("oo",540,190,220,120);
	//mo->opt->setBgColor(mo,0xffffff,0xff);
	gui_node_add_child_node_to_end(mo,main);

	mslide *slide = create_slide_widget("slide",0,0,1280,720);
	slide->opt->set_mv_dir(slide,SLIDE_NODE_MOVE_DIR_X);
	slide->opt->set_adsorb_point(&slide->data,1,550,200);
	gui_node_add_child_node_to_end(slide,main);


	mbox *mb = create_box_widget("box1",100,200,200,100);
	gui_node_add_child_node_to_end(mb,slide);

	mbutton *mu = create_button_widget("button1", 0,0,200,100);
	mu->opt->image_set(mu,"off.bmp","on.bmp");
	mu->opt->activate_event(mu,"onClick");
	gui_node_add_child_node_to_end(mu,mb);
	//gui_node_add_child_node_to_end(mu,slide);

	mrect *m;
	unsigned char alpha = 0xff;
	int colors[] = {0x696969, 0x6495ED, 0xADFF2F, 0xFFD700, 0x8B8B00, 0xFFC1C1, 0xFF6A6A,0xEE7942,0xFF1493};

	int i;
	for(i = 0; i < 9; i++){
		char name[16];
		sprintf(name,"box%d",i+2);
		mb = create_box_widget(name,50+200,0,200,100);
		mb->location_type = GUI_LOCATION_TYPE_PREV_BROTHERS;
		gui_node_add_child_node_to_end(mb,slide);

		if(i == 2){
			mbutton *mu = create_button_widget("button2", 0,0,200,100);
			mu->opt->image_set(mu,"off.bmp","on.bmp");
			mu->opt->activate_event(mu,"onClick");
			gui_node_add_child_node_to_end(mu,mb);
			continue;
		}

		sprintf(name,"rect%d",i+2);
		m = create_rect_widget(name, 0,0,200,100);
		m->location_type = GUI_LOCATION_TYPE_PARENT;
		//m->opt->setBgColor(m,colors[i],alpha);
		gui_node_add_child_node_to_end(m,mb);

		sprintf(name,"text%d",i+2);
		mtext *mt = create_text_widget(name,50,0,100,100);
		mt->opt->change_text(mt,name);
		textStyle style = {
			.textSize = 40,
			.textColor = 0xff000000,
			.textAlpha = 0xff,
			//.textBKColor = 0xffffff00,
			.textAlign = ALIGN_TO_CENTER,
			.transparentBKFlag = 1,
		};

		mt->opt->set_style(mt,&style);
		gui_node_add_child_node_to_end(mt,mb);

	}

	add_node_to_screen((mwidget*)main);
}

static void button_slideloop_test(void)
{
	mimage *main = create_image_widget("main",0,0,1280,720);
	main->opt->set_picture(main,"res/images1/pic0.jpg");

	//main->opt->activate_event(main,"onClick");

	mrect *mo = create_rect_widget("oo",540,190,220,120);
	//mo->opt->setBgColor(mo,0xffffff,0xff);
	gui_node_add_child_node_to_end(mo,main);

	mslideloop *slide = create_slideloop_widget("slide",0,0,1280,720);
	slide->opt->set_mv_dir(slide,SLIDELOOP_NODE_MOVE_DIR_X);
	slide->opt->set_gap(slide,100);
	slide->opt->set_adsorb_point(slide,1,550,200);
	gui_node_add_child_node_to_end(slide,main);


	mbox *mb = create_box_widget("box1",100,200,200,100);
	gui_node_add_child_node_to_end(mb,slide);

	mbutton *mu = create_button_widget("button1", 0,0,200,100);
	mu->opt->image_set(mu,"res/images1/off.bmp","res/images1/on.bmp");
	mu->opt->activate_event(mu,"onClick");
	gui_node_add_child_node_to_end(mu,mb);
	//gui_node_add_child_node_to_end(mu,slide);

	mrect *m;
	unsigned char alpha = 0xff;
	int colors[] = {0x696969, 0x6495ED, 0xADFF2F, 0xFFD700, 0x8B8B00, 0xFFC1C1, 0xFF6A6A,0xEE7942,0xFF1493};

	int i;
	for(i = 0; i < 9; i++){
		char name[16];
		sprintf(name,"box%d",i+2);
		mb = create_box_widget(name,50+200,0,200,100);
		//mb->location_type = GUI_LOCATION_TYPE_PREV_BROTHERS;
		gui_node_add_child_node_to_end(mb,slide);

		if(i == 2){
			mbutton *mu = create_button_widget("button2", 0,0,200,100);
			mu->opt->image_set(mu,"res/images1/off.bmp","res/images1/on.bmp");
			mu->opt->activate_event(mu,"onClick");
			gui_node_add_child_node_to_end(mu,mb);
			continue;
		}

		sprintf(name,"rect%d",i+2);
		m = create_rect_widget(name, 0,0,200,100);
		m->location_type = GUI_LOCATION_TYPE_PARENT;
		//m->opt->setBgColor(m,colors[i],alpha);
		gui_node_add_child_node_to_end(m,mb);

		sprintf(name,"text%d",i+2);
		mtext *mt = create_text_widget(name,50,0,100,100);
		mt->opt->change_text(mt,name);
		textStyle style = {
			.textSize = 40,
			.textColor = 0x000000,
			.textAlpha = 0xff,
			//.textBKColor = 0xffff00,
			.textAlign = ALIGN_TO_CENTER,
			.transparentBKFlag = 1,
		};

		mt->opt->set_style(mt,&style);
		gui_node_add_child_node_to_end(mt,mb);

	}

	add_node_to_screen((mwidget*)main);
}

static void orb_test(void)
{
	mimage *main = create_image_widget("main",0,0,480,480);
	main->opt->set_picture(main,"res/home_bg_1.png");

	mtext *mt = create_text_widget("time",20,40,301,120);
	textStyle style = {
		.textSize = 120,
		.textColor = 0xFFFFFFFF,
		//.textBKColor = 0xffffff00,
		.textAlpha = 0xff,
		.textAlign = ALIGN_TO_LEFT,
		.transparentBKFlag = 1,
		.fontType = 1,
	};
	mt->opt->set_style(mt,&style);
	mt->opt->change_text(mt,"08:07");
	gui_node_add_child_node_to_end(mt,main);

	mimage *mi = create_image_widget("weather",340,50,132,110);
	mi->opt->set_picture(mi,"res/weather_cloudy_becoming_fine.png");
	gui_node_add_child_node_to_end(mi,main);

	mt = create_text_widget("date",30,168,160,37);
	style.textSize = 26;
	style.fontType = 0;
	mt->opt->set_style(mt,&style);
	mt->opt->change_text(mt,"5月22日 周五");
	gui_node_add_child_node_to_end(mt,main);

	mt = create_text_widget("temp",380,168,56,40);
	style.textSize = 36;
	mt->opt->set_style(mt,&style);
	mt->opt->change_text(mt,"20°");
	gui_node_add_child_node_to_end(mt,main);

	mbox *mb = create_box_widget("musicbox",20,317,440,120);
	mrect *mr = create_rect_widget("musicrect",0,0,440,120);
	//mr->opt->setBgColor(mr,0x252525,0x99);
	//mr->opt->setRoundrect(mr,1,10,10);
	gui_node_add_child_node_to_end(mr,mb);
	mi = create_image_widget("musicimage",20,28,67,64);
	mi->opt->set_picture(mi,"res/music.png");
	gui_node_add_child_node_to_end(mi,mb);

	mbutton *mbn = create_button_widget("prev",236,40,40,40);
	mbn->opt->image_set(mbn,"res/icon_on_normal.png","res/icon_on_press.png");
	gui_node_add_child_node_to_end(mbn,mb);

	mbn = create_button_widget("next",380,40,40,40);
	mbn->opt->image_set(mbn,"res/icon_under_normal.png","res/icon_under_press.png");
	gui_node_add_child_node_to_end(mbn,mb);



	gui_node_add_child_node_to_end(mb,main);



	add_node_to_screen((mwidget*)main);

}

static void gdi_test(void)
{
	mrect *main = create_rect_widget("main",0,0,480,480);
	//main->opt->setBgColor(main,0xffffff,0xff);

	mbox *mb = create_box_widget("box",0,0,480,480);

	gui_node_add_child_node_to_end(mb,main);


	add_node_to_screen((mwidget*)main);
}

static void shade_test(void)
{
	//unsigned int color = 0xffffff;
	unsigned int color = 0;
	mrect *main = create_rect_widget("main",0,0,480,480);
	//main->opt->setBgColor(main,color,0xff);

	mrect *mr = create_rect_widget("rect",0,0,480,480);
	//mr->opt->setBgColor(mr,0x4A90E2,60);
	gui_node_add_child_node_to_end(mr,main);

	add_node_to_screen((mwidget*)main);

}

static void svg_test(void)
{
	mimage *main = create_image_widget("main",0,0,480,480);
	main->opt->set_picture(main,"res/home_bg_1.png");

	msvg *ms = create_svg_widget("svg",10,10,400,400);
	ms->opt->setSolidBrushColor(ms,0xffffff, 0xff);
	ms->opt->fillCircle(ms,230,30,20, 0);

	ms->opt->setPenWidth(ms,4);
	ms->opt->setPenColor(ms,0x00ff00, 0xff);
	ms->opt->line(ms, 10,10, 100,40);

	ms->opt->circle(ms, 250,50, 40);

	ms->opt->setSolidBrushColor(ms,0xffff00, 0x80);
	ms->opt->fillCircle(ms,100,100,40, 0);

	ms->opt->circleArc(ms,250,250,40,90,120);
	ms->opt->arcEx(ms,10,200,40,60,90,120);
	
	ms->opt->setSolidBrushColor(ms,0xffff00, 0x80);
	ms->opt->fillArc(ms,300,10,80,80,90,120, 0);

	ms->opt->setTextureBrushImage(ms, "res/light_bk.png");
	ms->opt->fillCircle(ms,300,100,40, 2);

	ms->opt->setPathGradientBrushCenterPoint(ms, 300,300);
	ms->opt->setPathGradientBrushCenterColor(ms, 0x000000, 0xee);
	ms->opt->setPathGradientBrushSurroundColor(ms, 0xffffff, 0xee, 10);
	ms->opt->setPathGradientBrushSurroundRect(ms, 300, 300, 50, 50);
	ms->opt->fillCircle(ms,300,300,40, 3);

	ms->opt->setLinearGradientBrushMode(ms, 0);
	ms->opt->setLinearGradientBrushRect(ms, 60, 300, 50, 50);
	ms->opt->setLinearGradientBrushColor(ms, 0xff00ff, 0xff, 20);
	ms->opt->fillCircle(ms,60,300,40, 4);

	POINT points[6]={110,150,150,110, 200,150, 175,200, 125, 100};
	ms->opt->fillPolygon(ms, &points, 4, 0);

	ms->opt->graphicDisplay(ms);
	//gui_node_add_child_node_to_end(ms,main);

	//ms = create_svg_widget("svg",0,150,480,300);
	//ms->opt->mgplus_test(ms);

	gui_node_add_child_node_to_end(ms,main);

	add_node_to_screen((mwidget*)main);
}

static void gif_test(void)
{
	mrect *main = create_rect_widget("main",0,0,480,480);
	//main->opt->setBgColor(main,0x000000,0xff);

	mgif *mg = create_gif_widget("gif", 0,0, 480,480);
	mg->opt->set_path(mg,"res/wakeup.gif");
	
	//gui_node_add_child_node_to_end(mg,main);

	//add_node_to_screen((mwidget*)main, &g_screen);
	add_node_to_screen((mwidget*)mg);
}


static void aircondision_test(void)
{

	gal_pixel pixs[21];
	color_shode(0x4A90E2,0x0,pixs,21);
	mrect *main = create_rect_widget("main",0,0,480,480);
	//main->opt->setBgColor(main,0x000000,0xff);
	main->opt->setCenterShode(main,1,240,0,240,pixs[10],0x0);

	msvg *ms = create_svg_widget("svg",21,24,30,25);
	ms->opt->setPenWidth(ms,3);
	ms->opt->setPenColor(ms,0xE4E4E4,0xff);
	ms->opt->line(ms,0,0,30,0);
	ms->opt->line(ms,0,11,30,11);
	ms->opt->line(ms,0,22,30,22);
	ms->opt->graphicDisplay(ms);
	gui_node_add_child_node_to_end(ms,main);

	mtext *mt = create_text_widget("title",72,19,244,34);
	mt->opt->change_text(mt,"VRV空调面板");
	textStyle style = {
		.textSize = 28,
		.textColor = 0xFFFFFFFF,
		//.textBKColor = 0xffffff00,
		.textAlpha = 0xff,
		.textAlign = ALIGN_TO_LEFT,
		.transparentBKFlag = 1,
		.fontType = 0,
	};

	mt->opt->set_style(mt,&style);
	gui_node_add_child_node_to_end(mt,main);


	mbox *mb = create_box_widget("detailbox",156,102,167,28);
	//mb->opt->set_alpha(mb,1,0x56);
	mtext *detail = create_text_widget("detail",0,0,167,28);
	detail->opt->change_text(detail,"室内温度: 18℃ ");
	style.textSize = 24;
	style.textColor = 0xE4E4E4;
	style.textAlign = ALIGN_TO_TOP;
	detail->opt->set_style(detail,&style);
	gui_node_add_child_node_to_end(detail,mb);
	gui_node_add_child_node_to_end(mb,main);
	//gui_node_add_child_node_to_end(detail,main);

	mt = create_text_widget("temp",164,142,153,138);
	mt->opt->change_text(mt,"18");
	style.textSize = 100;
	style.textAlign = ALIGN_TO_LEFT;
	style.fontType = TEXT_FONT_TYPE_ROBOTOREGULAR;
	mt->opt->set_style(mt,&style);
	gui_node_add_child_node_to_end(mt,main);

	mt = create_text_widget("temp1",318,148,32,32);
	mt->opt->change_text(mt,"℃ ");
	style.textSize = 22;
	style.fontType = 0;
	mt->opt->set_style(mt,&style);
	gui_node_add_child_node_to_end(mt,main);

	maircondision *ma = create_aircondision_widget("air",66,305,348,60);
	ma->opt->set_select_index(ma,12);
	gui_node_add_child_node_to_end(ma,main);

	mt = create_text_widget("mode",54,419,52,34);
	mt->opt->change_text(mt,"制冷");
	style.textSize = 26;
	style.textColor = 0x4A90E2;
	mt->opt->set_style(mt,&style);
	gui_node_add_child_node_to_end(mt,main);

	mt = create_text_widget("fan",214,419,52,34);
	mt->opt->change_text(mt,"弱风");
	mt->opt->set_style(mt,&style);
	gui_node_add_child_node_to_end(mt,main);

	mt = create_text_widget("fan",361,419,78,34);
	mt->opt->change_text(mt,"左右风");
	mt->opt->set_style(mt,&style);
	gui_node_add_child_node_to_end(mt,main);

	add_node_to_screen((mwidget*)main);
	
}

static void toggleswitch_test(void)
{
	mrect *main = create_rect_widget("main",0,0,480,480);
	//main->opt->setBgColor(main,0x000000,0xff);

	mtoggleswitch *mto = create_toggleswitch_widget("toggle",100,100,70,32);
	gui_node_add_child_node_to_end(mto,main);
	
	add_node_to_screen((mwidget*)main);

}


static void table_test(void){
	mrect *main = create_rect_widget("main",0,0,480,480);
	mtable *mtb = create_table_widget("table",0,0,480,480);
	gui_node_add_child_node_to_end(mtb,main);

	mtb->opt->SetLayout(mtb,3,3);

	mbutton *btn;
	mtext  *mtxt;
	char   text_str[16] = {0};
	for (int i = 0; i < 12; i++){

		btn = create_button_widget("button",0,0,200,200);
		btn->opt->image_set(btn,"res/home_bg_1.png","res/music.png");
		gui_node_add_child_node_to_end(btn,mtb);

		mtxt = create_text_widget("text",10,10,100,100);
		sprintf(text_str, "image%02d", i + 1);
		mtxt->opt->change_text(mtxt, text_str);
		gui_node_add_child_node_to_end(mtxt,btn);
	}

	add_node_to_screen((mwidget*)main);
}


void widget_test(void)
{
	//button_test();	
	//text_test();
	//slide_test();
	//rect_test();
	//box_test();
	//box_slide_test();
	//button_slide_test();
	//button_slideloop_test();
	
	//orb_test();
	
	//gdi_test();
	
	//shade_test();
	
	//svg_test();
	
	//gif_test();
	
	//aircondision_test();
	
	//toggleswitch_test();

	table_test();
}

