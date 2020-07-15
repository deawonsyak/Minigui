
#include "screen.h"
#include "image_widget.h"
#include "button_widget.h"
#include "text_widget.h"
#include "slide_widget.h"
#include "rect_widget.h"
#include "box_widget.h"
#include "slideloop_widget.h"
#include "slidescreen_widget.h"
#include "svg_widget.h"
#include "gif_widget.h"
#include "aircondision_widget.h"
#include "switch_widget.h"

struct {
	mwidget			*main;
	mwidget			*light;

	maircondision	*air;
	mtext			*air_temp;
	mrect			*air_bk;
	mbox			*music_box;
	mbox			*music_img_box;
	mwidget			*gif;
	mbutton			*play;
	char			play_flag;

}orb_t = {0};

static mwidget *gif_page(void)
{
	mrect *main = create_rect_widget("main_gif",0,0,480,480);
	//main->opt->setBgColor(main,0x000000,0xff);
	main->opt->activate_event(main,"onClick");

	mgif *mg = create_gif_widget("gif", 0,0, 480,480);
	mg->opt->set_path(mg,"res/wakeup.gif");
	gui_node_add_child_node_to_end(mg,main);

	return (mwidget*)main;
	
}

static mwidget *light_page(void)
{
	mrect *main = create_rect_widget("lightbx",0,0,480,480);
	//main->opt->setBgColor(main,0x000000,0xff);
	mbox *main_box = create_box_widget("light_box",0,0,480,480);
	gui_node_add_child_node_to_end(main,main_box);

	mtext *mt = create_text_widget("lighttext",28,19,100,41);
	textStyle style = {
		.textSize = 28,
		.textColor = 0xFFFFFF,
		.textAlign = ALIGN_TO_LEFT,
		.transparentBKFlag = 1,
		.textAlpha = 0xff,
		.fontType = TEXT_FONT_TYPE_PINGFANGREGULAR,
	};
	mt->opt->set_style(mt,&style);
	mt->opt->change_text(mt,"灯");
	gui_node_add_child_node_to_end(mt,main);

	mslide *msl = create_slide_widget("light_slide",0,72,480,480-72);
	msl->opt->set_mv_dir(msl,SLIDE_NODE_MOVE_DIR_Y);
	gui_node_add_child_node_to_end(msl,main);

	int i;
	for(i = 0; i < 12; i++){
		int x = 0,y = 10+120;
		mbox *mb;
		if(i == 0){
			x = 0;
			y = 0;
			mb = create_box_widget("lightbox",x,y,480,120);
		}else{
			mb = create_box_widget("lightbox",x,y,480,120);
			mb->location_type = GUI_LOCATION_TYPE_PREV_BROTHERS;
		}
		char name[20];
		sprintf(name,"lightrect%d",i*2);
		mrect *mte = create_rect_widget(name,20,0,215,120);
		//mte->opt->setRoundrect(mte,1,10,10);
		//mte->opt->setBgColor(mte,0xffffff,38);
		gui_node_add_child_node_to_end(mte,mb);

		mimage *mi = create_image_widget(name,20,28,64,64);
		mi->opt->set_picture(mi,"res/light_bk.png");
		gui_node_add_child_node_to_end(mi,mte);

		sprintf(name,"light%d",i*2);
		mi = create_image_widget(name,32,40,40,40);
		mi->opt->set_picture(mi,"res/panel_icon_chandelier.png");
		gui_node_add_child_node_to_end(mi,mte);

		sprintf(name,"lighttext%d",i*2);
		mt = create_text_widget(name,96,30,72,33);
		style.textSize = 22;
		style.textColor= 0xffffff;
		style.fontType = TEXT_FONT_TYPE_PINGFANGREGULAR;
		mt->opt->set_style(mt,&style);
		sprintf(name,"吊灯%d",i*2);
		mt->opt->change_text(mt,name);
		gui_node_add_child_node_to_end(mt,mte);

		mt = create_text_widget(name,96,56,27,38);
		style.textSize = 16;
		//style.textColor= 0x646464;
		//style.textColor = 0x545454;
		//style.textColor = 0x545454;
		style.textColor = 0xffffff;
		style.fontType = TEXT_FONT_TYPE_ROBOTOREGULAR;
		mt->opt->change_text(mt,"开");
		mt->opt->set_style(mt,&style);
		gui_node_add_child_node_to_end(mt,mte);

		sprintf(name,"lightrect%d",i*2);
		mte = create_rect_widget(name,245,0,215,120);
		//mte->opt->setRoundrect(mte,1,10,10);
		//mte->opt->setBgColor(mte,0xffffff,38);
		gui_node_add_child_node_to_end(mte,mb);

		mi = create_image_widget(name,20,28,64,64);
		mi->opt->set_picture(mi,"res/light_bk.png");
		gui_node_add_child_node_to_end(mi,mte);

		sprintf(name,"light%d",i*2);
		mi = create_image_widget(name,32,40,40,40);
		mi->opt->set_picture(mi,"res/panel_icon_chandelier.png");
		gui_node_add_child_node_to_end(mi,mte);

		sprintf(name,"lighttext%d",i*2);
		mt = create_text_widget(name,96,30,72,33);
		style.textSize = 22;
		style.textColor= 0xffffff;
		style.fontType = TEXT_FONT_TYPE_PINGFANGREGULAR;
		mt->opt->set_style(mt,&style);
		sprintf(name,"吊灯%d",i*2+1);
		mt->opt->change_text(mt,name);
		gui_node_add_child_node_to_end(mt,mte);

		mt = create_text_widget(name,96,56,27,38);
		style.textSize = 16;
		//style.textColor= 0x646464;
		//style.textColor = 0x848484;
		style.textColor = 0xffffff;
		style.fontType = TEXT_FONT_TYPE_ROBOTOREGULAR;
		mt->opt->change_text(mt,"开");
		mt->opt->set_style(mt,&style);
		gui_node_add_child_node_to_end(mt,mte);

		gui_node_add_child_node_to_end(mb,msl);

	}



	return main_box;
}

static mwidget *music_box_create(void)
{
	mbox *mb = create_box_widget("musicbox",20,317,440,120);
	mb->opt->activate_event(mb,"onClick");
	orb_t.music_box = mb;

	mrect *mr = create_rect_widget("musicrect",0,0,440,120);
	//mr->opt->setBgColor(mr,0x252525,0x99);
	//mr->opt->setRoundrect(mr,1,10,10);
	gui_node_add_child_node_to_end(mr,mb);
	mbox *mgb = create_box_widget("musicimagebox",20,24,72,72);
	mimage *mi = create_image_widget("musicimage",0,0,72,72);
	mi->opt->set_picture(mi,"res/music.png");
	gui_node_add_child_node_to_end(mi,mgb);
	orb_t.music_img_box = mgb;
	gui_node_add_child_node_to_end(mgb,mb);

	mtext *mt = create_text_widget("musicname",104,30,102,33);
	mt->opt->change_text(mt,"That Girl");
	textStyle style = {
		.textSize = 24,
		.textColor = 0xFFFFFFFF,
		//.textBKColor = 0xffffff00,
		.textAlign = ALIGN_TO_LEFT,
		.transparentBKFlag = 1,
		.fontType = TEXT_FONT_TYPE_PINGFANGREGULAR,
	};
	mt->opt->set_style(mt,&style);
	gui_node_add_child_node_to_end(mt,mb);

	mt = create_text_widget("musicact",104,63,79,25);
	style.textSize = 18;
	style.textColor = 0x848484;
	mt->opt->set_style(mt,&style);
	mt->opt->change_text(mt,"Sky High");
	gui_node_add_child_node_to_end(mt,mb);



	mbutton *mbn = create_button_widget("prev",236,30,60,60);
	mbn->opt->image_set(mbn,"res/icon_on_normal.png","res/icon_on_press.png");
	gui_node_add_child_node_to_end(mbn,mb);

	mbn = create_button_widget("play",308,30,60,60);
	mbn->opt->image_set(mbn,"res/icon_play_normal2.png","res/icon_play_press2.png");
	mbn->opt->image_set(mbn,"res/icon_play_normal.jpeg","res/icon_play_press.png");
	mbn->opt->activate_event(mbn,"onClick");
	gui_node_add_child_node_to_end(mbn,mb);
	orb_t.play = mbn;

	mbn = create_button_widget("next",380,30,60,60);
	mbn->opt->image_set(mbn,"res/icon_under_normal.png","res/icon_under_press.png");
	gui_node_add_child_node_to_end(mbn,mb);

	return (mwidget*)mb;

}

static mwidget *main_page_create(void)
{
	//mbox *main_box = create_box_widget("main_box",0,0,480,480);
	mimage *main = create_image_widget("main",0,0,480,480);
	main->opt->set_picture(main,"res/home_bg_1.png");
	//gui_node_add_child_node_to_end(main,main_box);

	mbox *mb = create_box_widget("timeb",20,40,301,120);
	mtext *mt = create_text_widget("time",0,0,301,120);
	textStyle style = {
		.textSize = 120,
		.textColor = 0xFFFFFF,
		.textAlpha = 0xff,
		//.textBKColor = 0xffffff00,
		.textAlign = ALIGN_TO_LEFT,
		.transparentBKFlag = 1,
		.fontType = 1,
	};
	mt->opt->set_style(mt,&style);
	mt->opt->change_text(mt,"08:07");
	gui_node_add_child_node_to_end(mb,main);
	gui_node_add_child_node_to_end(mt,mb);

	mimage *mi = create_image_widget("weather",340,50,132,110);
	mi->opt->set_picture(mi,"res/weather_cloudy_becoming_fine.png");
	gui_node_add_child_node_to_end(mi,main);

	mb = create_box_widget("dateb",30,168,160,37);
	mt = create_text_widget("date",0,0,160,37);
	style.textSize = 26;
	style.fontType = 0;
	mt->opt->set_style(mt,&style);
	mt->opt->change_text(mt,"5月22日 周五");
	gui_node_add_child_node_to_end(mb,main);
	gui_node_add_child_node_to_end(mt,mb);

	mb = create_box_widget("tempb",380,168,56,40);
	mt = create_text_widget("temp",0,0,56,40);
	style.textSize = 36;
	mt->opt->set_style(mt,&style);
	mt->opt->change_text(mt,"20°");
	gui_node_add_child_node_to_end(mb,main);
	gui_node_add_child_node_to_end(mt,mb);


	mb = (mbox*)music_box_create();

	gui_node_add_child_node_to_end(mb,main);


	//return (mwidget*)main_box;
	return (mwidget*)main;
}

gal_pixel pixs[21];
static mwidget *aircondision_page_create(void)
{
	color_shode(0x4A90E2,0x0,pixs,21);
	mrect *main = create_rect_widget("main",0,0,480,480);
	main->opt->set_property(main,"color","0");
	main->opt->setCenterShode(main,1,240,0,240,pixs[10],0x0);
	orb_t.air_bk = main;

	mbox *main_box = create_box_widget("main_box",0,0,480,480);
	gui_node_add_child_node_to_end(main,main_box);

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
		.textColor = 0xFFFFFF,
		//.textBKColor = 0xffffff00,
		.textAlpha = 0xff,
		.textAlign = ALIGN_TO_LEFT,
		.transparentBKFlag = 1,
		.fontType = 0,
	};

	mt->opt->set_style(mt,&style);
	gui_node_add_child_node_to_end(mt,main);

	mtoggleswitch *mto = create_toggleswitch_widget("switch",390,19,70,32);
	gui_node_add_child_node_to_end(mto,main);



	mbox *mb = create_box_widget("detailbox",156,102,167,28);
	//mb->opt->set_alpha(mb,1,0x56);
	mtext *detail = create_text_widget("detail",0,0,167,28);
	detail->opt->change_text(detail,"室内温度: 22℃ ");
	style.textSize = 24;
	style.textColor = 0xFFFFFF;
	style.textAlpha = 0x84;
	style.textAlign = ALIGN_TO_TOP;
	detail->opt->set_style(detail,&style);
	gui_node_add_child_node_to_end(detail,mb);
	gui_node_add_child_node_to_end(mb,main);

	mt = create_text_widget("temp",164,112,153,168);
	mt->opt->change_text(mt,"18");
	style.textSize = 138;
	style.textAlign = ALIGN_TO_TOP;
	style.textColor = 0xFFFFFF;
	style.textAlpha = 0xE4;
	style.fontType = TEXT_FONT_TYPE_STHeitiSCMedium;
	mt->opt->set_style(mt,&style);
	gui_node_add_child_node_to_end(mt,main);
	orb_t.air_temp = mt;

	mt = create_text_widget("temp1",318,148,32,32);
	mt->opt->change_text(mt,"℃ ");
	style.textSize = 22;
	style.fontType = TEXT_FONT_TYPE_PINGFANGREGULAR;
	mt->opt->set_style(mt,&style);
	gui_node_add_child_node_to_end(mt,main);

	maircondision *ma = create_aircondision_widget("air",66,305,348,60);
	ma->opt->set_select_index(ma,12);
	gui_node_add_child_node_to_end(ma,main);
	orb_t.air = ma;

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

	return (mwidget*)main_box;
}

static int orb_widget_event_handle(widget_event_t *event, void *param)
{
	printf("%s %d event:%s hd:%p\r\n",__func__,__LINE__,event->event,event->hd);
	if(event->hd == orb_t.air){
		char temp[10] = {0};
		sprintf(temp,"%d",orb_t.air->select_index+12);
		orb_t.air_temp->opt->change_text(orb_t.air_temp,temp);
	//main->opt->setCenterShode(main,1,240,0,240,pixs[10],0x0);
		orb_t.air_bk->opt->setCenterShode(orb_t.air_bk,1,240,0,240,pixs[orb_t.air->select_index],0x0);
	}else if(event->hd == orb_t.music_box){
		add_node_to_screen((mwidget*)orb_t.gif);
	}else if(event->hd == orb_t.gif){
		add_node_to_screen((mwidget*)orb_t.main);
	}else if(event->hd == orb_t.play){
		if(orb_t.play_flag == 0){
			orb_t.play->opt->image_set(orb_t.play,"res/icon_stop_normal2.png","res/icon_stop_press2.png");
			orb_t.music_img_box->opt->set_auto_rotate(orb_t.music_img_box,1,4);
		}else{
			orb_t.play->opt->image_set(orb_t.play,"res/icon_play_normal2.png","res/icon_play_press2.png");
			orb_t.music_img_box->opt->set_auto_rotate(orb_t.music_img_box,0,4);

		}
		orb_t.play_flag = !orb_t.play_flag;
	}


	return 0;
}

void orb_test(void)
{
	orb_t.gif = (mgif*)gif_page();

	mwidget *main = main_page_create();
	mwidget *aircondision = aircondision_page_create();

	mwidget *light = light_page();

	mslideloop *slide = create_slidescreen_widget("slidescreen",0,0,480,480);
	slide->opt->set_mv_dir(slide,SLIDESCREEN_NODE_MOVE_DIR_X);
	slide->opt->set_gap(slide,0);
	slide->opt->set_adsorb_point(slide,1,0,0);
	orb_t.main = slide;

	gui_node_add_child_node_to_end(main,slide);
	gui_node_add_child_node_to_end(light,slide);
	gui_node_add_child_node_to_end(aircondision,slide);

	add_node_to_screen((mwidget*)slide);
	//add_node_to_screen((mwidget*)light, &g_screen);

	screen_register_widget_event_handle(orb_widget_event_handle,NULL);
}
