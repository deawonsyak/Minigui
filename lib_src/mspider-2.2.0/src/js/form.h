#ifndef _FORM_H_
#define _FORM_H_

#ifdef JS_SUPPORT

/* note: the obj is form's JSObject !!! */
JSObject* getinputsmobj(JSContext *ctx, JSObject *obj, jsobject *findobj);

#endif

#endif /* _FORM_H_*/
