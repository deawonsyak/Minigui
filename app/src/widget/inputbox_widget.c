#include "inputbox_widget.h"
#include "screen.h"

#define cursor_height(textSize)	\
	textSize - 5;

#define cursor_width(cursor_height)	\
	(cursor_height >> 3) > 0 ? (cursor_height >> 3): 2;


struct borderType{
	const char *typeName;
	enum inputBorder border;
}borderSet[] = {
	{"Normal", NORMAL_INPUTBORDER},
	{"Simple", SIMPLE_INPUTBORDER}
};


static int inputboxPositonCalculate(minputbox *node, HDC hdc, unsigned int *format, RECT *textRect)
{
	if(format == NULL || textRect == NULL){
		return -1;
	}
	inputboxData *inputbox = node->inputbox;
	inputCursor *cursor = inputbox->cursor;
	int tipsFlag = 0;

	node->opt->get_location(node);
	
	SIZE size;
	memset(&size, 0, sizeof(SIZE));
	
	textStyle *inputStyle = NULL;
	char *drawText = NULL;	
	if(inputbox->dataLen == 0){
		drawText = inputbox->tipsText;
		inputStyle = inputbox->tipsStyle;
		tipsFlag = 1;
	}else{
		drawText = inputbox->inputText;
		inputStyle = inputbox->inputTextStyle;
	}

	GetTextExtent (hdc, drawText, -1, &size);

	int horizonFlag = 0, verticalFlag = 0;
	int textLeft = 0, textRight = 0, textTop = 0, textBottom = 0;
	if(inputStyle->textAlign & ALIGN_TO_LEFT){
		horizonFlag = 1;
		textLeft = node->start_abs_x;
		textRight = textLeft + size.cx;
	}else if(inputStyle->textAlign & ALIGN_TO_RIGHT){
		horizonFlag = 1;
		textRight = node->start_abs_x + node->w;
		textLeft = textRight - size.cx;
	}
	if(inputStyle->textAlign & ALIGN_TO_TOP){
		verticalFlag = 1;
		textTop = node->start_abs_y;
		textBottom = textTop + size.cy;
	}else if(inputStyle->textAlign & ALIGN_TO_BOTTOM){
		verticalFlag = 1;
		textBottom = node->start_abs_y + node->h;
		textTop = textBottom - size.cy;
	}
	if(inputStyle->textAlign & ALIGN_TO_CENTER || (horizonFlag == 0) || (verticalFlag == 0)){
		if(horizonFlag == 0){
			textLeft = node->start_abs_x + (node->w - size.cx)/2;
			textRight = textLeft + size.cx;
		}
		if(verticalFlag == 0){
			textTop = node->start_abs_y + (node->h - size.cy)/2;
			textBottom = textTop + size.cy;
		}
	}
//	printf("[%s %d] top: [%d] bottom: [%d] left: [%d] right: [%d]\n",__FILE__,__LINE__, textTop, textBottom, textLeft, textRight);
//	printf("[%s %d] showX: [%d] showY: [%d] trueW: [%d] trueH: [%d]\n",__FILE__,__LINE__, node->show_abs_x, node->show_abs_y, node->show_w, node->show_h);

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
	if(inputStyle->textAlign & ALIGN_TO_LEFT){
		cursor->x = (tipsFlag == 0 ? textRight+5: textLeft);		// +n调整光标显示效果
	}else{
		cursor->x = (tipsFlag == 0 ?textLeft-5: textRight);			// -n调整光标显示效果
		cursor->x -= cursor->w;
	}
	cursor->y = textTop+15;		//+n调整光标显示效果

	return 0;
}

int find_left_inputpoint(mwidget *node,HDC hdc)
{
	minputbox *m = (minputbox *)node;
	inputboxData *inputbox = m->inputbox;
	inputCursor *cursor = inputbox->cursor;

	char *drawText = inputbox->inputText;
	int gap = cursor->x - inputbox->touchX;
	
	SIZE size;
	memset(&size, 0, sizeof(SIZE));
	int fit_chars = 0;
	if(inputbox->inputPoint == 0){
		return 0;
	}
	int *pos_chars = (int *)widget_calloc(sizeof(int), inputbox->inputPoint);
	int *dx_chars = (int *)widget_calloc(sizeof(int), inputbox->inputPoint);
	GetTextExtentPoint(hdc, drawText, inputbox->inputPoint, ((unsigned)~0)>>1, &fit_chars, pos_chars, dx_chars, &size);

	int i = inputbox->inputPoint-1;
	if(gap < (size.cx - dx_chars[i])>>1)	//touch point is close to cursor point, change nothing
		goto end;
	for(; i > 0; i--){
		if(abs(size.cx - dx_chars[i] -gap) <= abs(size.cx - dx_chars[i-1] -gap))
			break;
	}
	inputbox->inputPoint = i;
	cursor->x -= (size.cx - dx_chars[i]);
end:
	if(pos_chars)
		widget_free(pos_chars);
	if(dx_chars)
		widget_free(dx_chars);
	return 0;
}

int find_right_inputpoint(mwidget *node,HDC hdc)
{
	minputbox *m = (minputbox *)node;
	inputboxData *inputbox = m->inputbox;
	inputCursor *cursor = inputbox->cursor;
	char *drawText = &inputbox->inputText[inputbox->inputPoint];
	int gap = inputbox->touchX - cursor->x;
	int drawTextLen = (int)strlen(drawText);
	int cursorMoveGap = 0;

	if(drawTextLen == 0)	//cursor is on the end, do nothing
		return 0;
	
	SIZE size;
	memset(&size, 0, sizeof(SIZE));
	int fit_chars = 0;
	int *pos_chars = (int *)widget_calloc(sizeof(int), inputbox->inputPoint);
	int *dx_chars = (int *)widget_calloc(sizeof(int), inputbox->inputPoint);
	GetTextExtentPoint(hdc, drawText, drawTextLen, ((unsigned)~0)>>1, &fit_chars, pos_chars, dx_chars, &size);
	int i = 0;
	
//	printf("[%s %d] gap: %d, cursor x: %d\n", __func__, __LINE__, gap, cursor->x);
	int firstCharWidth = drawTextLen > 1 ? dx_chars[1]: size.cx;
	if(gap < (firstCharWidth >> 1))	//touch point is close to cursor point, change nothing
		goto end;
	for(; i < drawTextLen-1; i++){
		if(abs(dx_chars[i] -gap) <= abs(dx_chars[i+1] -gap))
			break;
	}
	cursorMoveGap = dx_chars[i];
	if(abs(dx_chars[i] - gap) > abs(size.cx - gap)){
		i++;
		cursorMoveGap = size.cx;
	}
	
	inputbox->inputPoint += i;
	cursor->x += cursorMoveGap;
end:
	if(pos_chars)
		widget_free(pos_chars);
	if(dx_chars)
		widget_free(dx_chars);
	return 0;
}


int find_inputpoint(mwidget *node,HDC hdc)
{
	minputbox *m = (minputbox *)node;
	inputboxData *inputbox = m->inputbox;
	inputCursor *cursor = inputbox->cursor;
	if(inputbox->touchX == cursor->x){	//touch on the cursor, do nothing
		return 0;
	}
//	printf("[%s %d] inputPoint: %d, cursor x: %d\n", __func__, __LINE__, inputbox->inputPoint, cursor->x);
	if(inputbox->touchX < cursor->x){	//touch on the left of cursor
		find_left_inputpoint(node, hdc);
	}else{
		find_right_inputpoint(node, hdc);	//touch on the right of cursor
	}
//	printf("[%s %d] inputPoint: %d, cursor x: %d\n", __func__, __LINE__, inputbox->inputPoint, cursor->x);
	return 0;
}

static int leftTextPositonCalculate_onTouch(minputbox *node, HDC hdc, unsigned int *format, RECT *textRect)
{
	if(format == NULL || textRect == NULL){
		return -1;
	}
	inputboxData *inputbox = node->inputbox;
	inputCursor *cursor = inputbox->cursor;
	int tipsFlag = 0;

	node->opt->get_location(node);
	
	SIZE size;
	memset(&size, 0, sizeof(SIZE));
	
	char *drawText = inputbox->inputText;
	textStyle *inputStyle = inputbox->inputTextStyle;
#if 0
	printf("[%s %d]left text: ", __func__, __LINE__);
	for(int i = 0; i < inputbox->inputPoint; i++){
		printf("%c", drawText[i]);
	}
	printf("\n");
#endif

	GetTextExtent(hdc, drawText, inputbox->inputPoint, &size);

	int textLeft = 0, textRight = 0, textTop = 0, textBottom = 0;
//	printf("[%s %d] text width: %d, left width: %d\n", __func__, __LINE__, size.cx, inputbox->leftWidth);
	*format |= DT_RIGHT;
	*format |= DT_SINGLELINE;
	textRight = cursor->x;
	textLeft = node->start_abs_x;
	inputbox->oldLeftLen = size.cx;
	inputbox->oldCursorX = cursor->x;
	
	textTop = node->start_abs_y + (node->h - size.cy)/2;
	textBottom = textTop + size.cy;

	if(textRight <= node->show_abs_x || textLeft >= node->show_abs_x + node->show_w ||
		textBottom <= node->show_abs_y || textTop >= node->show_abs_y+node->show_h){
		*format = 0;
		return -1;
	}
	cursor->y = textTop+15;		//+n调整光标显示效果
	cursor->h = cursor->defaultH;
	if(cursor->y < node->show_abs_y){
		cursor->y = node->show_abs_y;
		cursor->h -= (node->show_abs_y-cursor->y);
	}
	if(cursor->y + cursor->h > node->show_abs_y + node->show_h){
		cursor->h -= (cursor->y + cursor->h - (node->show_abs_y + node->show_h));
	}
	
	textRect->left = node->show_abs_x > textLeft ? node->show_abs_x: textLeft;
	textRect->right = node->show_abs_x + node->show_w < textRight? node->show_abs_x + node->show_w: textRight;
	textRect->top = node->show_abs_y > textTop? node->show_abs_y: textTop;
	textRect->bottom = node->show_abs_y + node->show_h < textBottom? node->show_abs_y + node->show_h: textBottom;
	return 0;
}


static int leftTextPositonCalculate(minputbox *node, HDC hdc, unsigned int *format, RECT *textRect)
{
	if(format == NULL || textRect == NULL){
		return -1;
	}
	inputboxData *inputbox = node->inputbox;
	inputCursor *cursor = inputbox->cursor;
	int tipsFlag = 0;
	*format = 0;

	node->opt->get_location(node);
	
	SIZE size;
	memset(&size, 0, sizeof(SIZE));
	
	char *drawText = inputbox->inputText;
	textStyle *inputStyle = inputbox->inputTextStyle;

	GetTextExtent(hdc, drawText, inputbox->inputPoint, &size);
#if 0
	printf("[%s %d]left text: ", __func__, __LINE__);
	for(int i = 0; i < inputbox->inputPoint; i++){
		printf("%c", drawText[i]);
	}
	printf("\n");
#endif

	int textLeft = 0, textRight = 0, textTop = 0, textBottom = 0;
//	printf("[%s %d] old cursor x: %d, gap: %d, text width: %d\n", __func__, __LINE__, inputbox->oldCursorX, size.cx - inputbox->oldLeftLen, size.cx);
	if(size.cx < node->w){
		cursor->x = node->start_abs_x + size.cx;
		*format |= DT_LEFT;
	}else{
		cursor->x = inputbox->oldCursorX + (size.cx - inputbox->oldLeftLen);
		cursor->x = (cursor->x < node->show_abs_x + node->show_w) ? cursor->x: (node->show_abs_x + node->show_w - 10);
		cursor->x = (cursor->x > node->show_abs_x) ? cursor->x: node->show_abs_x+10;
		*format |= DT_RIGHT;
	}
	*format |= DT_SINGLELINE;
	textRight = cursor->x;
	textLeft = node->start_abs_x;
	inputbox->oldLeftLen = size.cx;
	inputbox->oldCursorX = cursor->x;
	
	textTop = node->start_abs_y + (node->h - size.cy)/2;
	textBottom = textTop + size.cy;

	if(textRight <= node->show_abs_x || textLeft >= node->show_abs_x + node->show_w ||
		textBottom <= node->show_abs_y || textTop >= node->show_abs_y+node->show_h){
		*format = 0;
		inputbox->cursorHideFlag = 1;
		return -1;
	}
	cursor->y = textTop+15;		//+n调整光标显示效果
	cursor->h = cursor->defaultH;
	if(cursor->y < node->show_abs_y){
		cursor->y = node->show_abs_y;
		cursor->h -= (node->show_abs_y-cursor->y);
	}
	if(cursor->y + cursor->h > node->show_abs_y + node->show_h){
		cursor->h -= (cursor->y + cursor->h - (node->show_abs_y + node->show_h));
	}
	
	textRect->left = node->show_abs_x > textLeft ? node->show_abs_x: textLeft;
	textRect->right = node->show_abs_x + node->show_w < textRight? node->show_abs_x + node->show_w: textRight;
	textRect->top = node->show_abs_y > textTop? node->show_abs_y: textTop;
	textRect->bottom = node->show_abs_y + node->show_h < textBottom? node->show_abs_y + node->show_h: textBottom;
	return 0;
}


static int rightTextPositonCalculate(minputbox *node, HDC hdc, unsigned int *format, RECT *textRect)
{
	if(format == NULL || textRect == NULL){
		return -1;
	}
	inputboxData *inputbox = node->inputbox;
	inputCursor *cursor = inputbox->cursor;
	int tipsFlag = 0;
	*format = 0;

//	printf("[%s %d] inputpoint: %d, right text: %s\n", __func__, __LINE__, inputbox->inputPoint, &inputbox->inputText[inputbox->inputPoint]);
	if(strlen(&inputbox->inputText[inputbox->inputPoint]) == 0){
		printf("input point at the end\n");
		return -1;
	}
	node->opt->get_location(node);
	
	SIZE size;
	memset(&size, 0, sizeof(SIZE));
	
	char *drawText = &inputbox->inputText[inputbox->inputPoint];
	textStyle *inputStyle = inputbox->inputTextStyle;

	GetTextExtent (hdc, drawText, -1, &size);

	int textLeft = 0, textRight = 0, textTop = 0, textBottom = 0;
	textRight = inputbox->cursor->x + size.cx;
	textLeft = inputbox->cursor->x;
	
	*format |= DT_LEFT;
	*format |= DT_SINGLELINE;
	textTop = node->start_abs_y + (node->h - size.cy)/2;
	textBottom = textTop + size.cy;

	if(textRight <= node->show_abs_x || textLeft >= node->show_abs_x + node->show_w ||
		textBottom <= node->show_abs_y || textTop >= node->show_abs_y+node->show_h){
		*format = 0;
		return -1;
	}
	
	textRect->left = node->show_abs_x > textLeft ? node->show_abs_x: textLeft;
	textRect->right = node->show_abs_x + node->show_w < textRight? node->show_abs_x + node->show_w: textRight;
	textRect->top = node->show_abs_y > textTop? node->show_abs_y: textTop;
	textRect->bottom = node->show_abs_y + node->show_h < textBottom? node->show_abs_y + node->show_h: textBottom;

	return 0;
}


static int tipsPositonCalculate(minputbox *node, HDC hdc, unsigned int *format, RECT *textRect)
{
	if(format == NULL || textRect == NULL){
		return -1;
	}
	inputboxData *inputbox = node->inputbox;
	inputCursor *cursor = inputbox->cursor;
	int tipsFlag = 0;

	node->opt->get_location(node);
	
	SIZE size;
	memset(&size, 0, sizeof(SIZE));
	
	char *drawText = inputbox->tipsText;
	textStyle *inputStyle = inputbox->tipsStyle;

	GetTextExtent(hdc, drawText, -1, &size);

	int textLeft = 0, textRight = 0, textTop = 0, textBottom = 0;
	textRight = node->start_abs_x + size.cx;
	textLeft = node->start_abs_x;
	
	textTop = node->start_abs_y + (node->h - size.cy)/2;
	textBottom = textTop + size.cy;

	if(textRight <= node->show_abs_x || textLeft >= node->show_abs_x + node->show_w ||
		textBottom <= node->show_abs_y || textTop >= node->show_abs_y+node->show_h){
		*format = 0;
		inputbox->cursorHideFlag = 1;
		return -1;
	}

	cursor->x = node->start_abs_x;
	cursor->y = textTop+15; 	//+n调整光标显示效果
	cursor->h = cursor->defaultH;
	if(cursor->x < node->show_abs_x || cursor->x >= node->show_abs_x + node->show_w);
		inputbox->cursorHideFlag = 1;
	if(cursor->y < node->show_abs_y){
		cursor->y = node->show_abs_y;
		cursor->h -= (node->show_abs_y-cursor->y);
	}
	if(cursor->y + cursor->h > node->show_abs_y + node->show_h){
		cursor->h -= (cursor->y + cursor->h - (node->show_abs_y + node->show_h));
	}
	
	textRect->left = node->show_abs_x > textLeft ? node->show_abs_x: textLeft;
	textRect->right = node->show_abs_x + node->show_w < textRight? node->show_abs_x + node->show_w: textRight;
	textRect->top = node->show_abs_y > textTop? node->show_abs_y: textTop;
	textRect->bottom = node->show_abs_y + node->show_h < textBottom? node->show_abs_y + node->show_h: textBottom;
	return 0;
}


void drawInputboxText(mwidget *node,HDC hdc)
{
	minputbox *m = (minputbox*)node;
	
	unsigned int format = 0;
	RECT textRect;
	memset(&textRect, 0, sizeof(RECT));
	inputboxData *inputbox = m->inputbox;
	inputCursor *cursor = inputbox->cursor;
	int tipsFlag = 0;

	node->opt->get_location(node);
	
	SIZE size;
	memset(&size, 0, sizeof(SIZE));
	
	textStyle *inputStyle = NULL;
	if(inputbox->dataLen == 0){
		if((!tipsPositonCalculate(node, hdc, &format, &textRect))){
			DrawText(hdc,inputbox->tipsText,-1,&textRect,format);//DT_RIGHT | DT_BOTTOM | DT_SINGLELINE | DT_WORDBREAK);
		}
		inputbox->touchEventFlag = 0;
	}else{
		if(inputbox->touchEventFlag == 1){
			inputbox->cursorHideFlag = 0;
			find_inputpoint(node, hdc);
			inputbox->touchEventFlag = 0;
			if((!leftTextPositonCalculate_onTouch(node, hdc, &format, &textRect))){
				DrawText(hdc,inputbox->inputText,inputbox->inputPoint,&textRect,format);//DT_RIGHT | DT_BOTTOM | DT_SINGLELINE | DT_WORDBREAK);
			}
		}else{
			if((!leftTextPositonCalculate(node, hdc, &format, &textRect))){
				DrawText(hdc,inputbox->inputText,inputbox->inputPoint,&textRect,format);//DT_RIGHT | DT_BOTTOM | DT_SINGLELINE | DT_WORDBREAK);
			}
		}
		if((!rightTextPositonCalculate(node, hdc, &format, &textRect))){
			DrawText(hdc,&inputbox->inputText[inputbox->inputPoint],-1,&textRect,format);//DT_RIGHT | DT_BOTTOM | DT_SINGLELINE | DT_WORDBREAK);
		}
	}
}

void drawNormalInputBorder(minputbox *node, HDC hdc)
{
	SetPenColor(hdc, ~0);
	SetPenWidth(hdc, 2);
	//top
	LineEx(hdc, node->start_abs_x,node->start_abs_y, node->start_abs_x+node->w,node->start_abs_y);
	//bottom
	LineEx(hdc, node->start_abs_x,node->start_abs_y+node->h, node->start_abs_x+node->w,node->start_abs_y+node->h);
	//left
	LineEx(hdc, node->start_abs_x,node->start_abs_y, node->start_abs_x,node->start_abs_y+node->h);
	//right
	LineEx(hdc, node->start_abs_x+node->w,node->start_abs_y, node->start_abs_x+node->w,node->start_abs_y+node->h);
}

void drawSimpleInputBorder(minputbox *node, HDC hdc)
{
	SetPenColor(hdc, ~0);
	SetPenWidth(hdc, 2);
	//bottom
	LineEx(hdc, node->start_abs_x,node->start_abs_y+node->h, node->start_abs_x+node->w,node->start_abs_y+node->h);
}

void drawInputBorder(mwidget *node,HDC hdc)
{
	minputbox *m = (minputbox*)node;
	inputboxData *inputbox = m->inputbox;
	node->opt->get_location(node);

	switch(inputbox->border){
		case NORMAL_INPUTBORDER:
			drawNormalInputBorder(m, hdc);
			break;
		case SIMPLE_INPUTBORDER:
			drawSimpleInputBorder(m, hdc);
			break;
	}
	
}

static int inputbox_node_paint(mwidget *node,HDC hdc)
{
	if(node == NULL){
		return -1;
	}
	minputbox *m = (minputbox*)node;
#if 1
	if(m->inputbox == NULL){
		return -1;
	}
	
	node->opt->lock_data(node);

	inputboxData *inputbox = m->inputbox;
	unsigned int color = rgb2pixel(inputbox->cursor->color);
	color |= 0xff << 24;
	
	textStyle *inputStyle = NULL;
	char *drawText = NULL;	
	if(inputbox->dataLen == 0){
		drawText = inputbox->tipsText;
		inputStyle = inputbox->tipsStyle;
	}else{
		drawText = inputbox->inputText;
		inputStyle = inputbox->inputTextStyle;
	}

	drawInputBorder(node, hdc);

	if(inputStyle == NULL || drawText == NULL){
		goto end;
	}
	
	SetTextColor(hdc, inputStyle->textColor);
	if(strlen(drawText) > 0){
		if(1 == inputStyle->transparentBKFlag){
			SetBkMode(hdc,BM_TRANSPARENT);
		}else{
			SetBkColor(hdc,inputStyle->textBKColor);
		}
		SelectFont(hdc,inputStyle->textFONT);
		drawInputboxText(node, hdc);
	}

end:
	if(inputbox->cursorHideFlag == 0){
		SetPenColor(hdc, color);
		SetPenWidth(hdc, inputbox->cursor->w);
		LineEx(hdc, inputbox->cursor->x,inputbox->cursor->y, inputbox->cursor->x,inputbox->cursor->y+inputbox->cursor->h);
//		printf("[%s %d]x: %d, y: %d, w: %d, h: %d\n", __func__, __LINE__, inputbox->cursor->x,inputbox->cursor->y,inputbox->cursor->w,inputbox->cursor->h);
	}
	node->opt->unlock_data(node);
#endif

	return 0;
}

void inputboxDataFree(inputboxData **data)
{
	if(data == NULL || *data == NULL){
		return;
	}
	inputboxData *d = *data;
	if(d->inputText)
		free(d->inputText);
	if(d->inputTextStyle){
		textStyle_delete(d->inputTextStyle);
	}
	if(d->tipsStyle)
		textStyle_delete(d->inputTextStyle);
	if(d->tipsText)
		free(d->tipsText);
	free(d);
	*data = NULL;
}

static void inputbox_destroy(mwidget *node,widget_opt *clas)
{
	minputbox *m = (minputbox*)node;

	m->opt->lock_data(node);
	if(m->inputbox){
		inputboxDataFree(&m->inputbox);
	}
	m->opt->unlock_data(node);
	clas->parent->destroy(node,clas->parent);
}

int input_touch_event_handle(struct _widget *node, unsigned int flag, struct move_info *coor)
{
//	printf("[%s %d]inputbox get x: %d, y: %d, flag: %x\n", __func__, __LINE__, coor->coor.x, coor->coor.y, flag);
	if(flag & SCREEN_FORCE_FLAG_BUTTONDOWN){
		minputbox *m = (minputbox*)node;
		m->inputbox->touchX = coor->coor.x;
		m->inputbox->touchEventFlag = 1;
		m->opt->refresh(m);
	}
	
	return 0;
}

struct _widget* inputbox_keyboard_event_handle(struct _widget *node, char *key)
{
	if(node == NULL || key == NULL){
		printf("[%s %d]error: unusual handle!!!\n", __func__, __LINE__);
		return NULL;
	}
	minputbox *m = (minputbox *)node;
	inputboxData *data = m->inputbox;
	if(data == NULL)
		return NULL;
	int inputLen = (int)strlen(key);
	char tailData[128] = {0};
	m->opt->lock_data(m);
	data->oldCursorX = data->cursor->x;
	int tailLen = data->dataLen-data->inputPoint;
	if(tailLen > 0)
		memcpy(tailData, &data->inputText[data->inputPoint], tailLen);
	for(; inputLen > 0; inputLen--, key++){
		if(*key == 'c'){	//delete operation
			if(data->inputPoint > 0){
				data->inputPoint--;
				data->inputText[data->inputPoint] = '\0';
				data->dataLen--;
			}
			continue;
		}
		if(data->dataLen < data->maxLen){
			data->inputText[data->inputPoint++] = *key;
			data->dataLen++;
		}
	}
	if(tailLen > 0)
		memcpy(&data->inputText[data->inputPoint], tailData, tailLen);
	data->inputText[data->inputPoint + tailLen] = '\0';		//clean up the tail string
	m->opt->unlock_data(m);
	return m;
}


int inputbox_text_set(minputbox *m, char *text)
{
	if(m == NULL || text == NULL){
		return -1;
	}

	m->opt->lock_data((mwidget*)m);
	strcpy(m->inputbox->inputText,text);
	m->inputbox->dataLen = (int)strlen(text);
	m->inputbox->inputPoint = m->inputbox->dataLen;
	m->opt->unlock_data((mwidget*)m);

	m->opt->refresh(m);
	return 0;
}

char *inputbox_text_get(minputbox *m)
{
	if(m == NULL || m->inputbox->inputText == NULL){
		return NULL;
	}

	m->opt->lock_data(m);
	char *buf = widget_calloc(1,strlen(m->inputbox->inputText) + 1);
	strcpy(buf,m->inputbox->inputText);
	m->opt->unlock_data(m);
	return buf;
}

int inputbox_border_set(minputbox *m, char *text)
{
	if(m == NULL || text == NULL){
		return -1;
	}

	m->opt->lock_data((mwidget*)m);
	int i = 0;
	for(; i < sizeof(borderSet)/sizeof(struct borderType); i++){
		if(strcmp(text, borderSet[i].typeName) == 0){
			m->inputbox->border = borderSet[i].border;
		}
	}
	m->opt->unlock_data((mwidget*)m);

	m->opt->refresh(m);
	return 0;
}

char *inputbox_border_get(minputbox *m)
{
	if(m == NULL || m->inputbox->inputText == NULL){
		return NULL;
	}
	char *buf = NULL;

	m->opt->lock_data(m);
	int i = 0;
	for(; i < sizeof(borderSet)/sizeof(struct borderType); i++){
		if(m->inputbox->border == borderSet[i].border){
			buf = widget_calloc(1,strlen(borderSet[i].typeName) + 1);
			strcpy(buf,borderSet[i].typeName);
		}
	}
	m->opt->unlock_data(m);
	return buf;
}

int inputbox_tips_text_set(minputbox *m, char *text)
{
	if(m == NULL || text == NULL){
		return -1;
	}

	m->opt->lock_data((mwidget*)m);
	if(m->inputbox->tipsText != NULL)
		widget_free(m->inputbox->tipsText);
	m->inputbox->tipsText = (char *)widget_calloc(strlen(text)+1, 1);
	strcpy(m->inputbox->tipsText,text);

	m->opt->unlock_data((mwidget*)m);

	m->opt->refresh(m);
	return 0;
}

char *inputbox_tips_text_get(minputbox *m)
{
	if(m == NULL || m->inputbox->tipsText == NULL){
		return NULL;
	}

	m->opt->lock_data(m);
	char *buf = widget_calloc(1,strlen(m->inputbox->tipsText) + 1);
	strcpy(buf,m->inputbox->tipsText);
	m->opt->unlock_data(m);
	return buf;
}


int inputbox_font_set(minputbox *m, char *value){
	if (NULL == value){
		GUI_ERR("inputbox font set param is NULL\n");
		return -1;
	}
	m->inputbox->inputTextStyle->fontType = GetFontTypeFromFontStr(value);
	m->inputbox->inputTextStyle->textFONT = text_font(m->inputbox->inputTextStyle->textSize, value);
	return 0;
}

char *inputbox_font_get(minputbox *m){
	char *buf = widget_calloc(1,32);
	char *font_str = GetFontStrFromFontType(m->inputbox->inputTextStyle->fontType);
	sprintf(buf, "%s", font_str);
	
	return buf;
}

int inputbox_tips_font_set(minputbox *m, char *value){
	if (NULL == value){
		GUI_ERR("inputbox font set param is NULL\n");
		return -1;
	}
	m->inputbox->tipsStyle->fontType = GetFontTypeFromFontStr(value);
	m->inputbox->tipsStyle->textFONT = text_font(m->inputbox->tipsStyle->textSize, value);
	return 0;
}

char *inputbox_tips_font_get(minputbox *m){
	char *buf = widget_calloc(1,32);
	char *font_str = GetFontStrFromFontType(m->inputbox->tipsStyle->fontType);
	sprintf(buf, "%s", font_str);
	
	return buf;
}

int inputbox_textsize_set(minputbox *m, char *value){
	if (NULL == value){
		GUI_ERR("inputbox size set param is NULL\n");
		return -1;
	}
	printf("%s, %d value: %s\n", __func__, __LINE__, value);
	m->inputbox->inputTextStyle->textSize = atoi(value);
	m->inputbox->inputTextStyle->textFONT = text_font(m->inputbox->inputTextStyle->textSize, GetFontStrFromFontType(m->inputbox->inputTextStyle->fontType));
	m->inputbox->cursor->defaultH = cursor_height(m->inputbox->inputTextStyle->textSize);
	m->inputbox->cursor->w = cursor_width(m->inputbox->cursor->h);
	return 0;
}

char *inputbox_textsize_get(minputbox *m){
	char *buf = widget_calloc(1,12);
	sprintf(buf, "%d", m->inputbox->inputTextStyle->textSize);
	
	return buf;
}

int inputbox_tips_textsize_set(minputbox *m, char *value){
	if (NULL == value){
		GUI_ERR("inputbox size set param is NULL\n");
		return -1;
	}
	m->inputbox->tipsStyle->textSize = atoi(value);
	m->inputbox->tipsStyle->textFONT = text_font(m->inputbox->tipsStyle->textSize, GetFontStrFromFontType(m->inputbox->tipsStyle->fontType));

	return 0;
}

char *inputbox_tips_textsize_get(minputbox *m){
	char *buf = widget_calloc(1,12);
	sprintf(buf, "%d", m->inputbox->tipsStyle->textSize);
	
	return buf;
}


int inputbox_align_set(minputbox *m, char *value){
	if (NULL == value){
		GUI_ERR("inputbox align set param is NULL\n");
		return -1;
	}

	int alignType = GetAlignTypeFromAlignStr(value);
	if(alignType < 0){
		GUI_ERR("set inputbox align failed, param=%s\n", value);
		return -1;
	}
	m->inputbox->inputTextStyle->textAlign = alignType;

	return 0;
}

char *inputbox_align_get(minputbox *m){
	char *buf = widget_calloc(1,16);
	char *align_str = GetAlignStrFromAlignType(m->inputbox->inputTextStyle->textAlign);
	sprintf(buf, "%s", align_str);
	
	return buf;
}

int inputbox_tips_align_set(minputbox *m, char *value){
	if (NULL == value){
		GUI_ERR("inputbox align set param is NULL\n");
		return -1;
	}

	int alignType = GetAlignTypeFromAlignStr(value);
	if(alignType < 0){
		GUI_ERR("set inputbox align failed, param=%s\n", value);
		return -1;
	}
	m->inputbox->tipsStyle->textAlign = alignType;

	return 0;
}

char *inputbox_tips_align_get(minputbox *m){
	char *buf = widget_calloc(1,16);
	char *align_str = GetAlignStrFromAlignType(m->inputbox->tipsStyle->textAlign);
	sprintf(buf, "%s", align_str);
	
	return buf;
}

int inputbox_color_set(minputbox *m, char *value){
	if (NULL == value){
		GUI_ERR("inputbox color set param is NULL\n");
		return -1;
	}

	m->inputbox->inputTextStyle->textColor = strtoul(value, 0, 16);

	return 0;
}

char *inputbox_color_get(minputbox *m){
	char *buf = widget_calloc(1,12);
	sprintf(buf, "0x%06x", m->inputbox->inputTextStyle->textColor);
	
	return buf;
}

int inputbox_tips_color_set(minputbox *m, char *value){
	if (NULL == value){
		GUI_ERR("inputbox color set param is NULL\n");
		return -1;
	}

	m->inputbox->tipsStyle->textColor = strtoul(value, 0, 16);

	return 0;
}

char *inputbox_tips_color_get(minputbox *m){
	char *buf = widget_calloc(1,12);
	sprintf(buf, "0x%06x", m->inputbox->tipsStyle->textColor);
	
	return buf;
}

int inputbox_transparent_back_flag_set(minputbox *m, char *value){
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
	
	m->inputbox->inputTextStyle->transparentBKFlag = (unsigned char)transparentBackFlag;

	return 0;
}

char *inputbox_transparent_back_flag_get(minputbox *m){
	char *buf = widget_calloc(1,12);
	sprintf(buf, "%s", GetEnableStrFromEnable(m->inputbox->inputTextStyle->transparentBKFlag));
	
	return buf;
}

int inputbox_tips_transparent_back_flag_set(minputbox *m, char *value){
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
	
	m->inputbox->tipsStyle->transparentBKFlag = (unsigned char)transparentBackFlag;

	return 0;
}

char *inputbox_tips_transparent_back_flag_get(minputbox *m){
	char *buf = widget_calloc(1,12);
	sprintf(buf, "%s", GetEnableStrFromEnable(m->inputbox->tipsStyle->transparentBKFlag));
	
	return buf;
}


int inputbox_back_color_set(minputbox *m, char *value){
	if (NULL == value){
		GUI_ERR("text color set param is NULL\n");
		return -1;
	}

	unsigned int textBKColor = strtoul(value, 0, 16);
	//GUI_DEBUG("%s: textBKColor = 0x%02x\n", __func__, textBKColor);
	 m->inputbox->inputTextStyle->textBKColor = textBKColor;

	return 0;
}


char *inputbox_back_color_get(minputbox *m){
	char *buf = widget_calloc(1,12);
	sprintf(buf, "0x%06x", m->inputbox->inputTextStyle->textBKColor);
	
	return buf;
}

int inputbox_tips_back_color_set(minputbox *m, char *value){
	if (NULL == value){
		GUI_ERR("text color set param is NULL\n");
		return -1;
	}

	unsigned int textBKColor = strtoul(value, 0, 16);
	//GUI_DEBUG("%s: textBKColor = 0x%02x\n", __func__, textBKColor);
	 m->inputbox->tipsStyle->textBKColor = textBKColor;

	return 0;
}

char *inputbox_tips_back_color_get(minputbox *m){
	char *buf = widget_calloc(1,12);
	sprintf(buf, "0x%06x", m->inputbox->tipsStyle->textBKColor);
	
	return buf;
}

static void set_default_text_style(textStyle *style){
	style->fontType = TEXT_DEFAULT_FONT_TYPE; 
	style->textSize = TEXT_DEFAULT_TEXT_SIZE;
	style->textAlign = TEXT_DEFAULT_TEXT_ALIGN; 
	style->textColor = TEXT_DEFAULT_TEXT_COLOR;
	style->textAlpha = TEXT_DEFAULT_TEXT_ALPHA;
	style->transparentBKFlag = TEXT_DEFAULT_TRANSPARENTBK_FLAG;
	style->textBKColor = TEXT_DEFAULT_TEXT_BKCOLOR;
	style->underlineFlag = TEXT_DEFAULT_TEXT_UNDERLINE;	
}

void set_default_cursor_style(inputCursor *cursor)
{
	cursor->x = 0;
	cursor->y = 0;
	cursor->defaultH = cursor_height(TEXT_DEFAULT_TEXT_SIZE);
	cursor->w = cursor_width(cursor->h);
	cursor->color = 0xffffffff;
}

void create_inputbox_data(minputbox *m)
{
	inputboxData *data = (inputboxData *)widget_calloc(sizeof(inputboxData), 1);
	data->maxLen = 128;
	data->inputText = (char *)widget_calloc(1, data->maxLen);
	data->inputTextStyle = (textStyle *)widget_calloc(sizeof(textStyle), 1);
	set_default_text_style(data->inputTextStyle);
	data->inputPoint = 0;
	data->tipsStyle = (textStyle *)widget_calloc(sizeof(textStyle), 1);
	set_default_text_style(data->tipsStyle);
	data->cursor = (inputCursor *)widget_calloc(sizeof(inputCursor), 1);
	set_default_cursor_style(data->cursor);
	m->inputbox = data;
}

_WIDGET_GENERATE(inputbox,widget)
