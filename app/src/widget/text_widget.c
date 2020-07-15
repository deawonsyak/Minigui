
#include "text_widget.h"
#include "screen.h"

StyleInfo m_FontInfo[]={
	{"PingFangRegular", TEXT_FONT_TYPE_PINGFANGREGULAR},
	{"robotoregular", TEXT_FONT_TYPE_ROBOTOREGULAR},
	{"STHeitiSCMedium", TEXT_FONT_TYPE_STHeitiSCMedium},
};

StyleInfo m_AlignInfo[]={
	{"left", ALIGN_TO_LEFT},
	{"right", ALIGN_TO_RIGHT},
	{"top", ALIGN_TO_TOP},
	{"bottom", ALIGN_TO_BOTTOM},
	{"center", ALIGN_TO_CENTER},
};

StyleInfo m_EnableInfo[]={
	{"disable", TEXT_PROPERTY_DISABLE},
	{"enable", TEXT_PROPERTY_ENABLE},
};

PLOGFONT text_font(int size, char *name)
{
	int i = 0;
	for(; i < sizeof(m_FontInfo)/sizeof(StyleInfo); i++){
		if(strcmp(name, m_FontInfo[i].styleStr) == 0)
			break;
	}
	i = (i == sizeof(m_FontInfo)/sizeof(StyleInfo) ? 0 : i);		//如果名字不匹配，给一个默认名字
	return CreateLogFont("ttf", m_FontInfo[i].styleStr, "UTF-8",		//注意：字符集一定要选择UTF-8
				FONT_WEIGHT_BOOK, FONT_SLANT_ROMAN,
				FONT_FLIP_NIL, FONT_OTHER_AUTOSCALE,
				FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, size, 0);
}


int GetFontTypeFromFontStr(char *font_str){
	if(NULL == font_str){
		GUI_ERR("font str is NULL, get font type failed\n");
		return -1;
	}

	int max_font_count = (sizeof(m_FontInfo) / sizeof(StyleInfo));
	for(int i = 0; i < max_font_count; i++){
		if (0 == memcmp(font_str, m_FontInfo[i].styleStr, strlen(m_FontInfo[i].styleStr))){
			return m_FontInfo[i].styleType;
		}
	}

	return -1;
}

char* GetFontStrFromFontType(int font_type){
	int max_font_count = (sizeof(m_FontInfo) / sizeof(StyleInfo));
	for (int index = 0; index < max_font_count; index++){
		if (m_FontInfo[index].styleType == font_type){
			return m_FontInfo[index].styleStr;
		}
	}

	return NULL;
}


int GetAlignTypeFromAlignStr(char *align_str){
	if(NULL == align_str){
		GUI_ERR("font str is NULL, get font type failed\n");
		return -1;
	}

	int max_font_count = (sizeof(m_AlignInfo) / sizeof(StyleInfo));
	for(int i = 0; i < max_font_count; i++){
		if (0 == memcmp(align_str, m_AlignInfo[i].styleStr, strlen(m_AlignInfo[i].styleStr))){
			return m_AlignInfo[i].styleType;
		}
	}

	return -1;
}

char* GetAlignStrFromAlignType(int text_align){
	int max_font_count = (sizeof(m_AlignInfo) / sizeof(StyleInfo));
	for (int index = 0; index < max_font_count; index++){
		if (m_AlignInfo[index].styleType == text_align){
			return m_AlignInfo[index].styleStr;
		}
	}

	return NULL;
}

int GetEnableFromEnableStr(char* property_str){
	if (NULL == property_str){
		GUI_ERR("text property str is NULL\n");
		return -1;
	}

	int max_enable_count = (sizeof(m_EnableInfo) / sizeof(StyleInfo));
	for(int i = 0; i < max_enable_count; i++){
		if (0 == memcmp(property_str, m_EnableInfo[i].styleStr, strlen(m_EnableInfo[i].styleStr))){
			return m_EnableInfo[i].styleType;
		}
	}

	return -1;
}

char* GetEnableStrFromEnable(int property){
	int max_enable_count = (sizeof(m_EnableInfo) / sizeof(StyleInfo));
	for(int i = 0; i < max_enable_count; i++){
		if (m_EnableInfo[i].styleType == property){
			return m_EnableInfo[i].styleStr;
		}
	}

	return NULL;
}


static int textPositonCalculate(mwidget *node, HDC hdc, unsigned int *format, RECT *textRect)
{
	mtext *m = (mtext*)node;
	if(format == NULL || textRect == NULL){
		return -1;
	}

	node->opt->get_location(node);
	
	SIZE size;
	memset(&size, 0, sizeof(SIZE));
	GetTextExtent (hdc, m->text, -1, &size);

	int horizonFlag = 0, verticalFlag = 0;
	int textLeft = 0, textRight = 0, textTop = 0, textBottom = 0;
	if(m->style){
		if(m->style->textAlign & ALIGN_TO_LEFT){
			horizonFlag = 1;
			textLeft = node->start_abs_x;
			textRight = textLeft + size.cx;
		}else if(m->style->textAlign & ALIGN_TO_RIGHT){
			horizonFlag = 1;
			textRight = node->start_abs_x + node->w;
			textLeft = textRight - size.cx;
		}
		if(m->style->textAlign & ALIGN_TO_TOP){
			verticalFlag = 1;
			textTop = node->start_abs_y;
			textBottom = textTop + size.cy;
		}else if(m->style->textAlign & ALIGN_TO_BOTTOM){
			verticalFlag = 1;
			textBottom = node->start_abs_y + node->h;
			textTop = textBottom - size.cy;
		}
		if(m->style->textAlign & ALIGN_TO_CENTER || (horizonFlag == 0) || (verticalFlag == 0)){
			if(horizonFlag == 0){
				textLeft = node->start_abs_x + (node->w - size.cx)/2;
				textRight = textLeft + size.cx;
			}
			if(verticalFlag == 0){
				textTop = node->start_abs_y + (node->h - size.cy)/2;
				textBottom = textTop + size.cy;
			}
		}
	}

	if(textRight <= node->show_abs_x || textLeft >= node->show_abs_x + node->show_w){
		*format = 0;
		return -1;
	}else if(textLeft >= node->show_abs_x){
		*format |= DT_LEFT;
		*format |= DT_SINGLELINE;
	}else if(textRight > node->show_abs_x && textRight <= node->show_abs_x + node->show_w){
		*format |= DT_RIGHT;
		*format |= DT_SINGLELINE;
	}
	textLeft = node->show_abs_x > textLeft ? node->show_abs_x: textLeft;
	textRight = node->show_abs_x + node->show_w < textRight? node->show_abs_x + node->show_w: textRight;

	if(textBottom <= node->show_abs_y || textTop >= node->show_abs_y + node->show_h){
		*format = 0;
		return -1;
	}else if(textTop > node->show_abs_y){
		*format |= DT_TOP;
		*format |= DT_SINGLELINE;
	}else if(textBottom < node->show_abs_y + node->show_h){
		*format |= DT_BOTTOM;
		*format |= DT_SINGLELINE;
	}
	textTop = node->show_abs_y > textTop? node->show_abs_y: textTop;
	textBottom = node->show_abs_y + node->show_h < textBottom? node->show_abs_y + node->show_h: textBottom;
	textRect->top = textTop;
	textRect->bottom = textBottom;
	textRect->left = textLeft;
	textRect->right = textRight;

	return 0;
}

static int text_node_paint(mwidget  *node,HDC hdc)
{
	mtext *m = (mtext*)node;
	//printf("%s %d\n",__func__,__LINE__);
	unsigned int format = 0;
	
	node->opt->lock_data(node);
	if(m->text != NULL && strlen(m->text) > 0){
		RECT textRect;

		memset(&textRect, 0, sizeof(RECT));

		HDC memHdc = screen_create_hdc(node->w,node->h);	
		if(m->style){
			SetMemDCColorKey (hdc, MEMDC_FLAG_SRCCOLORKEY , RGB2Pixel (hdc, 0x01, 0x01, 0x01));

			SetMemDCAlpha(memHdc,MEMDC_FLAG_SRCALPHA,m->style->textAlpha);
		
			SetPenColor(memHdc,m->style->textColor);
			SetTextColor(memHdc, m->style->textColor);

			//GUI_DEBUG("SetBkColor transparentBKFlag = %d textBKColor = 0x%06x\n",m->style->transparentBKFlag,  m->style->textBKColor);
			if(1 == m->style->transparentBKFlag){
				SetBkMode(memHdc,BM_TRANSPARENT);
				SetMemDCColorKey (memHdc, MEMDC_FLAG_SRCCOLORKEY , RGB2Pixel (memHdc, 0x01, 0x01, 0x01));
				SetBrushColor(memHdc, RGB2Pixel (memHdc, 0x01, 0x01, 0x01));
				FillBox(memHdc, 0, 0, node->w, node->h);
			}else{
				SetBkColor(memHdc,m->style->textBKColor);
			}
			
			SelectFont(memHdc,m->style->textFONT);
		}
				
		if((!textPositonCalculate(node, memHdc, &format, &textRect))){
			textRect.right = node->w;
			textRect.bottom = node->h;
			textRect.left = textRect.left - node->start_abs_x;
			textRect.top = textRect.top - node->start_abs_y;
			DrawText(memHdc, m->text,-1,&textRect,format);//DT_RIGHT | DT_BOTTOM | DT_SINGLELINE | DT_WORDBREAK);
			BitBlt (memHdc, 0, 0, node->w, node->h, hdc, node->start_abs_x, node->start_abs_y, 0);
			
		}

		screen_destroy_hdc(memHdc);

	}
	node->opt->unlock_data(node);

	return 0;
}

void textStyle_delete(textStyle *style)
{
	if(style != NULL){
		style->refer_num--;
		if(style->refer_num <= 0){
			if(style->textFONT){
				DestroyLogFont(style->textFONT);
			}
			widget_free(style);
		}
	}
}

static void text_destroy(mwidget *node,widget_opt *clas)
{
	mtext *m = (mtext*)node;	

	m->opt->lock_data(node);
	if(m->text){
		widget_free(m->text);
	}
	m->text = NULL;
	textStyle_delete(m->style);
	m->style = NULL;
	m->opt->unlock_data(node);

	clas->parent->destroy(node,clas->parent);
}

static void change_text(mtext *m, char *text)
{
	if(m == NULL){
		return;
	}

	m->opt->lock_data((mwidget*)m);
	if(m->text){
		widget_free(m->text);
		m->text = NULL;
	}
	m->text = widget_calloc(1,strlen(text)+1);
	strcpy(m->text,text);

	m->opt->unlock_data((mwidget*)m);

	m->opt->refresh(m);
}

int text_text_set(mtext *m, char *text)
{
	if(m == NULL || text == NULL){
		return -1;
	}

	m->opt->lock_data((mwidget*)m);
	if(m->text){
		widget_free(m->text);
		m->text = NULL;
	}
	m->text = widget_calloc(1,strlen(text)+1);
	strcpy(m->text,text);

	m->opt->unlock_data((mwidget*)m);

	m->opt->refresh(m);
	return 0;
}



char *text_text_get(mtext *m)
{
	if(m == NULL || m->text == NULL){
		return NULL;
	}

	m->opt->lock_data(m);
	char *buf = widget_calloc(1,strlen(m->text) + 1);
	strcpy(buf,m->text);
	m->opt->unlock_data(m);
	return buf;
}

static void get_default_style(textStyle *style){
	style->fontType = TEXT_DEFAULT_FONT_TYPE; 
	style->textSize = TEXT_DEFAULT_TEXT_SIZE;
	style->textAlign = TEXT_DEFAULT_TEXT_ALIGN; 
	style->textColor = TEXT_DEFAULT_TEXT_COLOR;
	style->textAlpha = TEXT_DEFAULT_TEXT_ALPHA;
	style->transparentBKFlag = TEXT_DEFAULT_TRANSPARENTBK_FLAG;
	style->textBKColor = TEXT_DEFAULT_TEXT_BKCOLOR;
	style->underlineFlag = TEXT_DEFAULT_TEXT_UNDERLINE;	
}

static void set_default_style(mtext *m){
	textStyle style;
	memset(&style, 0, sizeof(textStyle));

	get_default_style(&style);
	m->opt->set_style(m, &style);
}



static void set_style(mtext *m, textStyle *style)
{
	if(m == NULL || style == NULL){
		return;
	}


	
	textStyle *new_style = widget_calloc(1,sizeof(textStyle));
	new_style->textAlign = style->textAlign;
	new_style->textSize = style->textSize;
	new_style->textColor = style->textColor;
	new_style->textBKColor = style->textBKColor;
	new_style->textAlpha = style->textAlpha;
	new_style->underlineFlag = style->underlineFlag;
	new_style->transparentBKFlag = style->transparentBKFlag;
	new_style->weight = style->weight;
	new_style->fontType = style->fontType;
	if(new_style->weight == 0){
		new_style->weight = FONT_WEIGHT_BOOK;
	}

	new_style->refer_num = 1;

	if(new_style->fontType == TEXT_FONT_TYPE_PINGFANGREGULAR){
		new_style->textFONT = CreateLogFont("ttf", "PingFangRegular", "UTF-8",		//注意：字符集一定要选择UTF-8
				new_style->weight, FONT_SLANT_ROMAN,
				FONT_FLIP_NIL, FONT_OTHER_AUTOSCALE,
				FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, new_style->textSize, 0);
	}else if(new_style->fontType == TEXT_FONT_TYPE_ROBOTOREGULAR){
		new_style->textFONT = CreateLogFont("ttf", "robotoregular", "UTF-8",		//注意：字符集一定要选择UTF-8
				new_style->weight, FONT_SLANT_ROMAN,
				FONT_FLIP_NIL, FONT_OTHER_AUTOSCALE,
				FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, new_style->textSize, 0);
	}else if(new_style->fontType == TEXT_FONT_TYPE_STHeitiSCMedium){
		new_style->textFONT = CreateLogFont("ttf", "STHeitiSCMedium", "UTF-8",		//注意：字符集一定要选择UTF-8
				new_style->weight, FONT_SLANT_ROMAN,
				FONT_FLIP_NIL, FONT_OTHER_AUTOSCALE,
				FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, new_style->textSize, 0);

	}

	if(NULL == new_style->textFONT) {
		GUI_ERR("create font error!  \n");
		textStyle_delete(new_style);
		return;
	}

	m->opt->lock_data((mwidget*)m);
	if(m->style){
		textStyle_delete(m->style);			
		m->style = NULL;
	}
	m->style = new_style;
	m->opt->unlock_data((mwidget*)m);
	m->opt->refresh((mwidget*)m);
}

int text_style_set(mtext *m, char *value){
	if (NULL == value){
		GUI_ERR("text style set param is NULL\n");
		return -1;
	}

	char *tmp = value;
	tmp = strchr(tmp,',');
	
	if (NULL == tmp){
		GUI_ERR("set text style failed, style is only font, param=%s\n", value);
		return -1;
	}

	textStyle stTextStyle;
	memset(&stTextStyle, 0, sizeof(textStyle));
	get_default_style(&stTextStyle);

	int fontStrLen = strlen(value) - strlen(tmp) ;
	char *fontStr = widget_calloc(1, fontStrLen + 1);
	memset(fontStr, 0, fontStrLen + 1);
	memcpy(fontStr,value, fontStrLen);

	int fontType = GetFontTypeFromFontStr(fontStr);
	if(fontType < 0){
		GUI_ERR("set text style failed, font is not support, param=%s\n", value);
	}else{
		stTextStyle.fontType = (unsigned char)fontType;
	}

	tmp++;
	stTextStyle.textSize = atoi(tmp);
	
	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("set text style failed, align is NULL, param=%s\n", value);
		goto set;
	}
	tmp++;
	
	char *tmp1 = tmp;
	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("set text style failed, not set text color, param=%s\n", value);
		goto set;
	}
	
	int alignStrLen = strlen(tmp1) - strlen(tmp);
	char *alignStr = widget_calloc(1, alignStrLen +1);
	memset(alignStr, 0, alignStrLen + 1);
	memcpy(alignStr,tmp1, alignStrLen);
	int alignType = GetAlignTypeFromAlignStr(alignStr);
	if(alignType < 0){
		GUI_ERR("set text style failed, algin is not right, param=%s\n", value);
	}else{
		stTextStyle.textAlign = alignType;
	}
	tmp++;	
	
	stTextStyle.textColor = strtoul(tmp, 0,16);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("set text style failed, transparent bk flag is NULL, param=%s\n", value);
		goto set;
	}
	tmp++;
	
	stTextStyle.textAlpha = atoi(tmp);
	
	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("set text style failed, transparent bk flag is NULL, param=%s\n", value);
		goto set;
	}
	tmp++;
	
	int transparentBKFlag = GetEnableFromEnableStr(tmp);
	if (transparentBKFlag < 0){
		GUI_ERR("set text style failed, transparent bk flag is invalid, param=%s\n", value);
	}else{
		stTextStyle.transparentBKFlag = transparentBKFlag;
	}

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("set text style failed, bk color is NULL, param=%s\n", value);
		goto set;
	}
	tmp++;
	
	stTextStyle.textBKColor = strtoul(tmp,0,16);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("set text style failed, under line flag is NULL, param=%s\n", value);
		goto set;
	}
	tmp++;

	int underlineFlag = GetEnableFromEnableStr(tmp);
	if (underlineFlag < 0){
		GUI_ERR("set text style failed, under line flag is invalid, param=%s\n", value);
	}else{
		stTextStyle.underlineFlag = underlineFlag;
	}
	goto set;

set:
	/*
	GUI_DEBUG("text style set:font:%s textSize:%d align:%s textColor:0x%02x textAlpha:0x%02x transparentBKFlag:%d textBKColor:0x%02x underlineFlag:%d\n", 
	fontStr, stTextStyle.textSize, alignStr, stTextStyle.textColor, stTextStyle.textAlpha, stTextStyle.transparentBKFlag,
	stTextStyle.textBKColor, stTextStyle.underlineFlag);
	*/

	m->opt->set_style(m, &stTextStyle);
	
	return 0;
}

char *text_style_get(mtext *m){
	char *buf = widget_calloc(1,256);
	char *font_str = GetFontStrFromFontType(m->style->fontType);
	char *align_str = GetAlignStrFromAlignType(m->style->textAlign);
	sprintf(buf, "%s,%d,%s,0x%06x,0x%02x,%s,0x%06x,%s", \
		font_str,m->style->textSize,align_str, m->style->textColor, m->style->textAlpha, \ 
		GetEnableStrFromEnable(m->style->transparentBKFlag), m->style->textBKColor, GetEnableStrFromEnable(m->style->underlineFlag));
	
	return buf;
}

int text_font_set(mtext *m, char *value){
	if (NULL == value){
		GUI_ERR("text font set param is NULL\n");
		return -1;
	}
	
	int fontType = GetFontTypeFromFontStr(value);
	if(fontType < 0){
		GUI_ERR("set text font failed, param=%s\n", value);
		return -1;
	}

	//GUI_DEBUG("%s: font = %s\n", __func__, value);

	if (m->style){
		m->style->fontType = (unsigned char)fontType;
		m->opt->set_style(m, m->style);
	}else{
		GUI_DEBUG("%s:m->style is NULL\n", __func__);
		textStyle stTextStyle;
		memset(&stTextStyle, 0, sizeof(textStyle));
		get_default_style(&stTextStyle);
		stTextStyle.fontType = (unsigned char)fontType;
		m->opt->set_style(m, &stTextStyle);
	}

	return 0;
}

char *text_font_get(mtext *m){
	char *buf = widget_calloc(1,32);
	char *font_str = GetFontStrFromFontType(m->style->fontType);
	
	sprintf(buf, "%s", font_str);
	
	return buf;
}

int text_size_set(mtext *m, char *value){
	if (NULL == value){
		GUI_ERR("text size set param is NULL\n");
		return -1;
	}

	int textSize = atoi(value);
	//GUI_DEBUG("%s: textSize = %d\n", __func__, textSize);
	if (m->style){
		m->style->textSize = textSize;
		m->opt->set_style(m, m->style);
	}else{
		GUI_ERR("%s:m->style is NULL\n", __func__);
		textStyle stTextStyle;
		memset(&stTextStyle, 0, sizeof(textStyle));
		get_default_style(&stTextStyle);
		stTextStyle.textSize = (unsigned char)textSize;
		m->opt->set_style(m, &stTextStyle);
	}

	return 0;
}

char *text_size_get(mtext *m){
	char *buf = widget_calloc(1,12);
	sprintf(buf, "%d", m->style->textSize);
	
	return buf;
}

int text_align_set(mtext *m, char *value){
	if (NULL == value){
		GUI_ERR("text align set param is NULL\n");
		return -1;
	}

	int alignType = GetAlignTypeFromAlignStr(value);
	if(alignType < 0){
		GUI_ERR("set text align failed, param=%s\n", value);
		return -1;
	}
	//GUI_DEBUG("%s: textAlign = %d\n", __func__, value);

	if (m->style){
		m->style->textAlign = (unsigned int)alignType;
		m->opt->set_style(m, m->style);
	}else{
		textStyle stTextStyle;
		memset(&stTextStyle, 0, sizeof(textStyle));
		get_default_style(&stTextStyle);
		stTextStyle.textAlign = (unsigned int)alignType;
		m->opt->set_style(m, &stTextStyle);
	}

	return 0;
}

char *text_align_get(mtext *m){
	char *buf = widget_calloc(1,16);
	char *align_str = GetAlignStrFromAlignType(m->style->textAlign);
	sprintf(buf, "%s", align_str);
	
	return buf;
}

int text_color_set(mtext *m, char *value){
	if (NULL == value){
		GUI_ERR("text color set param is NULL\n");
		return -1;
	}

	unsigned int textColor = strtoul(value, 0, 16);
	//GUI_DEBUG("%s: textColor = 0x%02x\n", __func__, textColor);
	if (m->style){
		m->style->textColor = textColor;
		m->opt->set_style(m, m->style);
	}else{
		textStyle stTextStyle;
		memset(&stTextStyle, 0, sizeof(textStyle));
		get_default_style(&stTextStyle);
		stTextStyle.textColor = (unsigned char)textColor;
		m->opt->set_style(m, &stTextStyle);
	}

	return 0;
}

char *text_color_get(mtext *m){
	char *buf = widget_calloc(1,12);
	sprintf(buf, "0x%06x", m->style->textColor);
	
	return buf;

}


int text_alpha_set(mtext *m, char *value){
	if (NULL == value){
		GUI_ERR("text color set param is NULL\n");
		return -1;
	}

	unsigned int textAlpha = strtoul(value, 0, 16);
	//GUI_DEBUG("%s: textAlpha = 0x%02x\n", __func__, textAlpha);
	if (m->style){
		m->style->textAlpha = textAlpha;
		m->opt->set_style(m, m->style);
	}else{
		textStyle stTextStyle;
		memset(&stTextStyle, 0, sizeof(textStyle));
		get_default_style(&stTextStyle);
		stTextStyle.textAlpha = (unsigned char)textAlpha;
		m->opt->set_style(m, &stTextStyle);
	}

	return 0;
}

char *text_alpha_get(mtext *m){
	char *buf = widget_calloc(1,12);
	sprintf(buf, "%d", m->style->textAlpha);
	
	return buf;
}


int text_transparent_back_flag_set(mtext *m, char *value){
	if (NULL == value){
		GUI_ERR("text transparent back flag set param is NULL\n");
		return -1;
	}

	int transparentBackFlag = GetEnableFromEnableStr(value);
	if (transparentBackFlag < 0){
		GUI_ERR("text transparent back flag set failed, value = %s\n", value);
		return -1;
	}

	//GUI_DEBUG("%s: transparentBackFlag = %d\n", __func__, transparentBackFlag);
	
	if (m->style){
		m->style->transparentBKFlag = (unsigned char)transparentBackFlag;
		m->opt->set_style(m, m->style);
	}else{
		textStyle stTextStyle;
		memset(&stTextStyle, 0, sizeof(textStyle));
		get_default_style(&stTextStyle);
		stTextStyle.transparentBKFlag = (unsigned char)transparentBackFlag;
		m->opt->set_style(m, &stTextStyle);
	}

	return 0;
}

char *text_transparent_back_flag_get(mtext *m){
	char *buf = widget_calloc(1,12);
	sprintf(buf, "%s", GetEnableStrFromEnable(m->style->transparentBKFlag));
	
	return buf;
}


int text_back_color_set(mtext *m, char *value){
	if (NULL == value){
		GUI_ERR("text color set param is NULL\n");
		return -1;
	}

	unsigned int textBKColor = strtoul(value, 0, 16);

	//GUI_DEBUG("%s: textBKColor = 0x%02x\n", __func__, textBKColor);
	
	if (m->style){
		m->style->textBKColor = textBKColor;
		m->opt->set_style(m, m->style);
	}else{
		textStyle stTextStyle;
		memset(&stTextStyle, 0, sizeof(textStyle));
		get_default_style(&stTextStyle);
		stTextStyle.textBKColor = (unsigned char)textBKColor;
		m->opt->set_style(m, &stTextStyle);
	}

	return 0;
}


char *text_back_color_get(mtext *m){
	char *buf = widget_calloc(1,12);
	sprintf(buf, "0x%06x", m->style->textBKColor);
	
	return buf;
}

int text_underline_set(mtext *m, char *value){
	if (NULL == value){
		GUI_ERR("text color set param is NULL\n");
		return -1;
	}

	int underlineFlag = GetEnableFromEnableStr(value);
	if (underlineFlag < 0){
		GUI_ERR("text underline set failed, value = %s\n", value);
		return -1;
	}

	//GUI_DEBUG("%s: underlineFlag = 0x%02x\n", __func__, underlineFlag);
	
	if (m->style){
		m->style->underlineFlag = (unsigned char)underlineFlag;
		m->opt->set_style(m, m->style);
	}else{
		textStyle stTextStyle;
		memset(&stTextStyle, 0, sizeof(textStyle));
		get_default_style(&stTextStyle);
		stTextStyle.underlineFlag = (unsigned char)underlineFlag;
		m->opt->set_style(m, &stTextStyle);
	}

	return 0;
}

char *text_underline_get(mtext *m){

	char *buf = widget_calloc(1,12);
	sprintf(buf, "%s", GetEnableStrFromEnable(m->style->underlineFlag));
	
	return buf;
}



_WIDGET_GENERATE(text,widget)
