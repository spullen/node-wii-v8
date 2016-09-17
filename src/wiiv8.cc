// WiiV8.cc
#include <v8.h>
#include <node.h>

#include <bluetooth/bluetooth.h>
#include "cwiid.h"

#include <stdlib.h>
#include <iostream>

#include "../include/wiiv8.h"

using namespace v8;
using namespace node;
using namespace std;

namespace WiiV8 {

    using v8::Context;
    using v8::Function;
    using v8::FunctionCallbackInfo;
    using v8::FunctionTemplate;
    using v8::Isolate;
    using v8::Local;
    using v8::Number;
    using v8::Object;
    using v8::Persistent;
    using v8::String;
    using v8::Value;

    Persistent <Function> WiiV8::constructor;
    Persistent <Function> WiiV8::emitterCallback;


#define ARRAY_SIZE(a)  ((sizeof(a) / sizeof(*(a))) / static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

    int cwiid_set_err(cwiid_err_t *err);

    void WiiV8_cwiid_err(struct wiimote *wiimote, const char *str, va_list ap) {
        (void)wiimote;

        // TODO move this into a error object, so we can return it in the error objects
        vfprintf(stdout, str, ap);
        fprintf(stdout, "\n");
    }


    void UV_NOP(uv_work_t* req) { /* No operation */ }

    /****************************************************************
    * Protected
    ****************************************************************/
    WiiV8::WiiV8() {
        wiimote = NULL;
    };

    WiiV8::~WiiV8() {
        Disconnect();
    };

    /****************************************************************
    * Public
    ****************************************************************/

    void WiiV8::Init(Isolate *isolate) {

        HandleScope handleScope(isolate);

//        cwiid_set_err(&WiiV8_cwiid_err);

        // Prepare constructor template
        Local <FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
        tpl->SetClassName(String::NewFromUtf8(isolate, "WiiV8"));
        tpl->InstanceTemplate()->SetInternalFieldCount(1);

        NODE_SET_PROTOTYPE_METHOD(tpl, "connect", Connect);
        NODE_SET_PROTOTYPE_METHOD(tpl, "disconnect", Disconnect);
        NODE_SET_PROTOTYPE_METHOD(tpl, "requestStatus", RequestStatus);
        NODE_SET_PROTOTYPE_METHOD(tpl, "rumble", Rumble);
        NODE_SET_PROTOTYPE_METHOD(tpl, "led", Led);
        NODE_SET_PROTOTYPE_METHOD(tpl, "ir", IrReporting);
        NODE_SET_PROTOTYPE_METHOD(tpl, "acc", AccReporting);
        NODE_SET_PROTOTYPE_METHOD(tpl, "ext", ExtReporting);
        NODE_SET_PROTOTYPE_METHOD(tpl, "button", ButtonReporting);
        NODE_SET_PROTOTYPE_METHOD(tpl, "setEmitter", SetEmitter);


        constructor.Reset(isolate, tpl->GetFunction());

//        constructor.Set(v8::String::NewSymbol(IR_X_MAX), v8::Integer::New(CWIID_IR_X_MAX), static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete));

    }

    void WiiV8::NewInstance(const FunctionCallbackInfo <Value> &args) {
        Isolate *isolate = args.GetIsolate();

        const unsigned argc = 1;
        Local <Value> argv[argc] = {args[0]};
        Local <Function> cons = Local<Function>::New(isolate, constructor);
        Local <Context> context = isolate->GetCurrentContext();
        Local <Object> instance =
                cons->NewInstance(context, argc, argv).ToLocalChecked();

        args.GetReturnValue().Set(instance);
    }

    int WiiV8::Connect(bdaddr_t * mac){

//        fprintf(stdout,batostr(mac));

        bacpy(&this->mac, mac);

        if(!(this->wiimote = cwiid_open(&this->mac, CWIID_FLAG_MESG_IFC))) {
            return -1;
        }

        return 0;
    }

    int WiiV8::Disconnect() {
        if (this->wiimote) {
            if (cwiid_get_data(this->wiimote)) {
                cwiid_set_mesg_callback(this->wiimote, NULL);
                cwiid_set_data(this->wiimote, NULL);
            }
            cwiid_close(this->wiimote);
            this->wiimote = NULL;
        }

        return 0;
    }

    int WiiV8::RequestStatus() {
        assert(this->wiimote != NULL);

        if(cwiid_request_status(this->wiimote)) {
            return -1;
        }

        return 0;
    }

    int WiiV8::Rumble(bool on) {
        unsigned char rumble = on ? 1 : 0;

        assert(this->wiimote != NULL);

        if(cwiid_set_rumble(this->wiimote, rumble)) {
            return -1;
        }

        return 0;
    }

    int WiiV8::Led(int index, bool on) {
        int indexes[] = { CWIID_LED1_ON, CWIID_LED2_ON, CWIID_LED3_ON, CWIID_LED4_ON };

        assert(this->wiimote != NULL);

        if(cwiid_get_state(this->wiimote, &this->state)) {
            return -1;
        }

        int led = this->state.led;

        led = on ? led | indexes[index-1] : led & indexes[index-1];

        if(cwiid_set_led(this->wiimote, led)) {
            return -1;
        }

        return 0;
    }

/**
 * Turns on or off the particular modes passed
 */

    int WiiV8::Reporting(int mode, bool on) {
        assert(this->wiimote != NULL);

        if(cwiid_get_state(this->wiimote, &this->state)) {
            return -1;
        }

        int newmode = this->state.rpt_mode;

        newmode = on ? (newmode | mode) : (newmode & ~mode);

        if(cwiid_set_rpt_mode(this->wiimote, newmode)) {
            return -1;
        }

        return 0;
    }


    void WiiV8::Connect(const v8::FunctionCallbackInfo<v8::Value>& args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope handleScope(isolate);

        WiiV8* wiiV8 = ObjectWrap::Unwrap<WiiV8>(args.This());
        Local<Function> callback;

        if(args.Length() == 0 || !args[0]->IsString()) {
            fprintf(stdout, "MAC address is required and must be a String." );
//            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "MAC address is required and must be a String.")));
        }

        if(args.Length() == 1 || !args[1]->IsFunction()) {
            fprintf(stdout, "Callback is required and must be a Function." );
//            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Callback is required and must be a Function.")));
        }

        callback = Local<Function>::Cast(args[1]);

        connect_request* ar = new connect_request();
        ar->wiiV8 = wiiV8;

        String::Utf8Value mac(args[0]);
        str2ba(*mac, &ar->mac); // TODO Validate the mac and throw an exception if invalid

        ar->callback.Reset(isolate, callback);

//        ar->callback = Persistent<Function>::New(callback);

        wiiV8->Ref();

        uv_work_t* req = new uv_work_t;
        req->data = ar;
        int r = uv_queue_work(uv_default_loop(), req, UV_Connect, UV_AfterConnect);
        if (r != 0) {
            ar->callback.Reset();
            delete ar;
            delete req;

            wiiV8->Unref();
            fprintf(stdout, "Internal error: Failed to queue connect work" );

//            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Internal error: Failed to queue connect work")));
        }
    }

    void WiiV8::UV_Connect(uv_work_t* req) {
        connect_request* ar = static_cast<connect_request* >(req->data);

        assert(ar->wiiV8 != NULL);

        ar->err = ar->wiiV8->Connect(&ar->mac);
    }

    void WiiV8::UV_AfterConnect(uv_work_t* req, int status) {
        connect_request* ar = static_cast<connect_request* >(req->data);
        delete req;

        WiiV8 * wiiV8 = ar->wiiV8;
        Isolate* isolate = v8::Isolate::GetCurrent();

        HandleScope handleScope(isolate);

        Local<Value> argv[1] = { Integer::New(isolate, ar->err) };

        if (ar->err == 0) {
            // Setup the callback to receive events
            cwiid_set_data(wiiV8->wiimote, wiiV8);
            cwiid_set_mesg_callback(wiiV8->wiimote, WiiV8::HandleMessages);
        }

        wiiV8->Unref();

        TryCatch try_catch;

        Local<Function>::New(isolate, ar->callback)->
                Call(isolate->GetCurrentContext()->Global(), 1, argv);

//        ar->callback->Call(Null(isolate), 1, argv);
//        ar->callback->Call(Context::GetCurrent()->Global(), 1, argv);

        if(try_catch.HasCaught())
            FatalException(isolate, try_catch);

        ar->callback.Reset();

        delete ar;
    }

    void WiiV8::Disconnect(const v8::FunctionCallbackInfo<v8::Value>& args) {
        Isolate* isolate;
        isolate = args.GetIsolate();

        HandleScope handleScope(isolate);

        WiiV8* wiiV8 = ObjectWrap::Unwrap<WiiV8>(args.This());
        args.GetReturnValue().Set(Integer::New(isolate,wiiV8->Disconnect()));
    }
    // Callback from libcwiid's thread
    void WiiV8::HandleMessages(cwiid_wiimote_t *wiiV8, int len, union cwiid_mesg mesgs[], struct timespec *timestamp) {
        WiiV8 *self = const_cast<WiiV8*>(static_cast<const WiiV8*>(cwiid_get_data(wiiV8)));

        // There is a race condition where this might happen
        if (self == NULL)
            return;

        // Make a copy of the message
        struct message_request * req = (struct message_request *)malloc( sizeof(*req) + sizeof(req->mesgs) * (len - 1) );
        req->wiimote = self;
        req->timestamp = *timestamp;
        req->len = len;
        memcpy(req->mesgs, mesgs, len * sizeof(union cwiid_mesg));

        // We need to pass this over to the nodejs thread, so it can create V8 objects
        uv_work_t* uv = new uv_work_t;
        uv->data = req;
        int r = uv_queue_work(uv_default_loop(), uv, UV_NOP, WiiV8::HandleMessagesAfter);
        if (r != 0) {
            free(req);
            delete uv;
        }
    }
    // Callback from Nodejs's thread which calls one of the Handle*Message methods
    void WiiV8::HandleMessagesAfter(uv_work_t *req, int status) {
        message_request* r = static_cast<message_request* >(req->data);
        WiiV8 * self = r->wiimote;
        delete req;

        Isolate* isolate = Isolate::GetCurrent();

        for (int i = 0; i < r->len; i++) {
            switch(r->mesgs[i].type) {
                case CWIID_MESG_STATUS:
                    self->HandleStatusMessage(isolate, &r->timestamp, (cwiid_status_mesg *)&r->mesgs[i]);
                    break;

                case CWIID_MESG_BTN:
                    self->HandleButtonMessage(isolate, &r->timestamp, (cwiid_btn_mesg *)&r->mesgs[i]);
                    break;

                case CWIID_MESG_ACC:
                    self->HandleAccMessage(isolate, &r->timestamp, (cwiid_acc_mesg *)&r->mesgs[i]);
                    break;

                case CWIID_MESG_IR:
//                    self->HandleIRMessage(isolate, &r->timestamp, (cwiid_ir_mesg *)&r->mesgs[i]);
                    break;

                case CWIID_MESG_NUNCHUK:
                    self->HandleNunchukMessage(isolate, &r->timestamp, (cwiid_nunchuk_mesg *)&r->mesgs[i]);
                    break;

                case CWIID_MESG_ERROR:
                    self->HandleErrorMessage(isolate, &r->timestamp, (cwiid_error_mesg *)&r->mesgs[i]);
                    break;

                case CWIID_MESG_CLASSIC:
                    self->HandleClassicMessage(isolate, &r->timestamp, (cwiid_classic_mesg *)&r->mesgs[i]);
                    break;

                case CWIID_MESG_BALANCE:
                    self->HandleBalanceMessage(isolate, &r->timestamp, (cwiid_balance_mesg *)&r->mesgs[i]);
                    break;

                case CWIID_MESG_MOTIONPLUS:
                    self->HandleMotionPlusMessage(isolate, &r->timestamp, (cwiid_motionplus_mesg *)&r->mesgs[i]);
                    break;

                case CWIID_MESG_UNKNOWN:
                default:
                    break;
            }
        }
        free(r);
    }

    void WiiV8::HandleAccMessage(Isolate* isolate, struct timespec *ts, cwiid_acc_mesg * msg) {
        HandleScope handle_scope(isolate);

        Local<Object> pos = Object::New(isolate);   // Create array of x,y,z
        pos->Set(String::NewFromUtf8(isolate, "x"), Integer::New(isolate, msg->acc[CWIID_X]) );
        pos->Set(String::NewFromUtf8(isolate, "y"), Integer::New(isolate, msg->acc[CWIID_Y]) );
        pos->Set(String::NewFromUtf8(isolate, "z"), Integer::New(isolate, msg->acc[CWIID_Z]) );

        Local<Value> argv[2] = { String::NewFromUtf8(isolate, "acc"), pos };
        Local<Function>::New(isolate, emitterCallback)->
                Call(isolate->GetCurrentContext()->Global(), ARRAY_SIZE(argv), argv);
    }

    void WiiV8::HandleButtonMessage(Isolate* isolate, struct timespec *ts, cwiid_btn_mesg * msg) {
        HandleScope handle_scope(isolate);

        Local<Integer> btn = Integer::New(isolate, msg->buttons);

        Local<Value> argv[2] = { String::NewFromUtf8(isolate, "button"), btn };
        Local<Function>::New(isolate, emitterCallback)->
                Call(isolate->GetCurrentContext()->Global(), ARRAY_SIZE(argv), argv);
    }

    void WiiV8::HandleErrorMessage(Isolate* isolate, struct timespec *ts, cwiid_error_mesg * msg) {
        HandleScope handle_scope(isolate);

        Local<Integer> err = Integer::New(isolate, msg->error);

        Local<Value> argv[2] = { String::NewFromUtf8(isolate, "error"), err };

        switch(msg->error){
            case CWIID_ERROR_NONE:
                fprintf(stdout, "CWIID_ERROR_NONE" );
                fprintf(stdout, "\n");
                break;
            case CWIID_ERROR_DISCONNECT:
                fprintf(stdout, "CWIID_ERROR_DISCONNECT");
                fprintf(stdout, "\n");
                break;
            case CWIID_ERROR_COMM:
                fprintf(stdout, "CWIID_ERROR_COMM");
                fprintf(stdout, "\n");
                break;
        }


        Local<Function>::New(isolate, emitterCallback)->
                Call(isolate->GetCurrentContext()->Global(), ARRAY_SIZE(argv), argv);
    }

    void WiiV8::HandleNunchukMessage(Isolate* isolate, struct timespec *ts, cwiid_nunchuk_mesg * msg) {
        HandleScope handle_scope(isolate);

        Local<Object> stick = Object::New(isolate);
        stick->Set(String::NewFromUtf8(isolate, "x"), Integer::New(isolate, msg->stick[CWIID_X]));
        stick->Set(String::NewFromUtf8(isolate, "y"), Integer::New(isolate, msg->stick[CWIID_Y]));

        Local<Object> pos = Object::New(isolate);
        pos->Set(String::NewFromUtf8(isolate, "x"), Integer::New(isolate, msg->acc[CWIID_X]));
        pos->Set(String::NewFromUtf8(isolate, "y"), Integer::New(isolate, msg->acc[CWIID_Y]));
        pos->Set(String::NewFromUtf8(isolate, "z"), Integer::New(isolate, msg->acc[CWIID_Z]));

        Local<Object> data = Object::New(isolate);
        data->Set(String::NewFromUtf8(isolate, "stick"), stick);
        data->Set(String::NewFromUtf8(isolate, "acc"), pos);
        data->Set(String::NewFromUtf8(isolate, "buttons"), Integer::New(isolate, msg->buttons));

        Local<Value> argv[2] = { String::NewFromUtf8(isolate, "nunchuk"), data };
        Local<Function>::New(isolate, emitterCallback)->
                Call(isolate->GetCurrentContext()->Global(), ARRAY_SIZE(argv), argv);
    }

    void WiiV8::HandleClassicMessage(Isolate* isolate, struct timespec *ts, cwiid_classic_mesg * msg) {
        HandleScope handle_scope(isolate);

        Local<Object> lstick = Object::New(isolate);
        lstick->Set(String::NewFromUtf8(isolate, "x"), Integer::New(isolate, msg->l_stick[CWIID_X]));
        lstick->Set(String::NewFromUtf8(isolate, "y"), Integer::New(isolate, msg->l_stick[CWIID_Y]));

        Local<Object> rstick = Object::New(isolate);
        rstick->Set(String::NewFromUtf8(isolate, "x"), Integer::New(isolate, msg->r_stick[CWIID_X]));
        rstick->Set(String::NewFromUtf8(isolate, "y"), Integer::New(isolate, msg->r_stick[CWIID_Y]));

        Local<Object> data = Object::New(isolate);
        data->Set(String::NewFromUtf8(isolate, "leftStick"), lstick);
        data->Set(String::NewFromUtf8(isolate, "rightStick"), rstick);
        data->Set(String::NewFromUtf8(isolate, "left"), Integer::New(isolate, msg->l));
        data->Set(String::NewFromUtf8(isolate, "right"), Integer::New(isolate, msg->r));
        data->Set(String::NewFromUtf8(isolate, "buttons"), Integer::New(isolate, msg->buttons));

        Local<Value> argv[2] = { String::NewFromUtf8(isolate, "classic"), data };
        Local<Function>::New(isolate, emitterCallback)->
                Call(isolate->GetCurrentContext()->Global(), ARRAY_SIZE(argv), argv);
    }

    void WiiV8::HandleBalanceMessage(Isolate* isolate, struct timespec *ts, cwiid_balance_mesg * msg) {
        HandleScope handle_scope(isolate);

        Local<Object> data = Object::New(isolate);
        data->Set(String::NewFromUtf8(isolate, "rightTop"), Integer::New(isolate, msg->right_top));
        data->Set(String::NewFromUtf8(isolate, "rightBottom"), Integer::New(isolate, msg->right_bottom));
        data->Set(String::NewFromUtf8(isolate, "leftTop"), Integer::New(isolate, msg->left_top));
        data->Set(String::NewFromUtf8(isolate, "leftBottom"), Integer::New(isolate, msg->left_bottom));

        Local<Value> argv[2] = { String::NewFromUtf8(isolate, "balance"), data };
        Local<Function>::New(isolate, emitterCallback)->
                Call(isolate->GetCurrentContext()->Global(), ARRAY_SIZE(argv), argv);
    }

    void WiiV8::HandleMotionPlusMessage(Isolate* isolate, struct timespec *ts, cwiid_motionplus_mesg * msg) {
        HandleScope handle_scope(isolate);

        Local<Object> angle = Object::New(isolate);
        angle->Set(String::NewFromUtf8(isolate, "x"), Integer::New(isolate, msg->angle_rate[CWIID_X]));
        angle->Set(String::NewFromUtf8(isolate, "y"), Integer::New(isolate, msg->angle_rate[CWIID_Y]));
        angle->Set(String::NewFromUtf8(isolate, "z"), Integer::New(isolate, msg->angle_rate[CWIID_Z]));

        // Local<Object> speed = Object::New(isolate);
        // speed->Set(String::NewSymbol("x"), Integer::New(msg->low_speed[CWIID_X]));
        // speed->Set(String::NewSymbol("y"), Integer::New(msg->low_speed[CWIID_Y]));
        // speed->Set(String::NewSymbol("z"), Integer::New(msg->low_speed[CWIID_Z]));

        Local<Object> data = Object::New(isolate);
        data->Set(String::NewFromUtf8(isolate, "angleRate"), angle);
        //data->Set(String::NewSymbol("lowSpeed"), speed);

        Local<Value> argv[2] = { String::NewFromUtf8(isolate, "motionplus"), data };
        Local<Function>::New(isolate, emitterCallback)->
                Call(isolate->GetCurrentContext()->Global(), ARRAY_SIZE(argv), argv);
    }

    void WiiV8::HandleIRMessage(Isolate* isolate, struct timespec *ts, cwiid_ir_mesg * msg) {
        HandleScope handle_scope(isolate);

        Local<Array> poss = Array::New(isolate, CWIID_IR_SRC_COUNT);

        // Check IR data sources
        for(int i=0; i < CWIID_IR_SRC_COUNT; i++) {
            if (!msg->src[i].valid)
                break; // Once we find one invalid then we stop

            // Create array of x,y
            Local<Object> pos = Object::New(isolate);
            pos->Set(String::NewFromUtf8(isolate, "x"), Integer::New(isolate,  msg->src[i].pos[CWIID_X] ));
            pos->Set(String::NewFromUtf8(isolate, "y"), Integer::New(isolate,  msg->src[i].pos[CWIID_Y] ));
            pos->Set(String::NewFromUtf8(isolate, "size"), Integer::New(isolate,  msg->src[i].size ));

            poss->Set(Integer::New(isolate, i), pos);
        }

        Local<Value> argv[2] = { String::NewFromUtf8(isolate, "ir"), poss };
        Local<Function>::New(isolate, emitterCallback)->Call(Null(isolate), ARRAY_SIZE(argv), argv);
    }

    void WiiV8::HandleStatusMessage(Isolate* isolate, struct timespec *ts, cwiid_status_mesg * msg) {
        Local<Object> obj = Object::New(isolate);

        HandleScope handle_scope(isolate);

        obj->Set(String::NewFromUtf8(isolate, "battery"),    Integer::New(isolate, msg->battery));
        obj->Set(String::NewFromUtf8(isolate, "extensions"), Integer::New(isolate, msg->ext_type));

        Local<Value> argv[2] = { String::NewFromUtf8(isolate, "status"), obj };
        Local<Function>::New(isolate, emitterCallback)->
                Call(isolate->GetCurrentContext()->Global(), ARRAY_SIZE(argv), argv);
    }


    // The following methods turn things on and off
    void WiiV8::RequestStatus(const v8::FunctionCallbackInfo<v8::Value>& args) {
        Isolate* isolate;
        isolate = args.GetIsolate();

        HandleScope handle_scope(isolate);

        WiiV8* wiiV8 = ObjectWrap::Unwrap<WiiV8>(args.This());
        args.GetReturnValue().Set(Integer::New(isolate,wiiV8->RequestStatus()));
    }

    void WiiV8::Rumble(const v8::FunctionCallbackInfo<v8::Value>& args) {
        Isolate* isolate;
        isolate = args.GetIsolate();

        HandleScope handle_scope(isolate);

        WiiV8* wiiV8 = ObjectWrap::Unwrap<WiiV8>(args.This());

        if(args.Length() == 0 || !args[0]->IsBoolean()) {
//            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "On state is required and must be a Boolean.")));
        }

        bool on = args[0]->ToBoolean()->Value();
        args.GetReturnValue().Set(Integer::New(isolate,wiiV8->Rumble(on)));
    }

    void WiiV8::Led(const v8::FunctionCallbackInfo<v8::Value>& args) {
        Isolate* isolate;
        isolate = args.GetIsolate();

        HandleScope handle_scope(isolate);

        WiiV8* wiiV8 = ObjectWrap::Unwrap<WiiV8>(args.This());

        if(args.Length() == 0 || !args[0]->IsNumber()) {
//            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Index is required and must be a Number.")));
        }

        if(args.Length() == 1 || !args[1]->IsBoolean()) {
//            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "On state is required and must be a Boolean.")));
        }

        int index = args[0]->ToInteger()->Value();
        bool on = args[1]->ToBoolean()->Value();

        args.GetReturnValue().Set(Integer::New(isolate,wiiV8->Led(index, on)));
    }

    void WiiV8::IrReporting(const v8::FunctionCallbackInfo<v8::Value>& args) {
        Isolate* isolate;
        isolate = args.GetIsolate();

        HandleScope handle_scope(isolate);

        WiiV8* wiiV8 = ObjectWrap::Unwrap<WiiV8>(args.This());

        if(args.Length() == 0 || !args[0]->IsBoolean()) {
//            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "On state is required and must be a Boolean.")));
        }

        bool on = args[0]->ToBoolean()->Value();
        args.GetReturnValue().Set(Integer::New(isolate,wiiV8->Reporting(CWIID_RPT_IR, on)));
    }

    void WiiV8::AccReporting(const v8::FunctionCallbackInfo<v8::Value>& args) {
        Isolate* isolate;
        isolate = args.GetIsolate();

        HandleScope handle_scope(isolate);

        WiiV8* wiiV8 = ObjectWrap::Unwrap<WiiV8>(args.This());

        if(args.Length() == 0 || !args[0]->IsBoolean()) {
//            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "On state is required and must be a Boolean.")));
        }

        bool on = args[0]->ToBoolean()->Value();
        args.GetReturnValue().Set(Integer::New(isolate,wiiV8->Reporting(CWIID_RPT_ACC, on)));
    }

    void WiiV8::ExtReporting(const v8::FunctionCallbackInfo<v8::Value>& args) {
        Isolate* isolate;
        isolate = args.GetIsolate();

        HandleScope handle_scope(isolate);

        WiiV8* wiiV8 = ObjectWrap::Unwrap<WiiV8>(args.This());

        if(args.Length() == 0 || !args[0]->IsBoolean()) {
//            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "On state is required and must be a Boolean.")));
        }

        bool on = args[0]->ToBoolean()->Value();
        args.GetReturnValue().Set(Integer::New(isolate,wiiV8->Reporting(CWIID_RPT_EXT, on)));
    }

    void WiiV8::ButtonReporting(const v8::FunctionCallbackInfo<v8::Value>& args) {
        Isolate* isolate;
        isolate = args.GetIsolate();

        HandleScope handle_scope(isolate);

        WiiV8* wiiV8 = ObjectWrap::Unwrap<WiiV8>(args.This());

        if(args.Length() == 0 || !args[0]->IsBoolean()) {
//            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "On state is required and must be a Boolean.")));
        }

        bool on = args[0]->ToBoolean()->Value();
        args.GetReturnValue().Set(Integer::New(isolate,wiiV8->Reporting(CWIID_RPT_BTN, on)));
    }

    void WiiV8::SetEmitter(const v8::FunctionCallbackInfo <v8::Value> &args){
        Isolate* isolate;
        isolate = args.GetIsolate();

        HandleScope handle_scope(isolate);

        Local<Function> callback = Local<Function>::Cast(args[0]);
        WiiV8::emitterCallback.Reset(isolate, callback);
    }



    /****************************************************************
     * Private
    ****************************************************************/
//    void WiiV8::Emit(Isolate* isolate, Local<Value> argv){
//        Local<Function>::New(isolate, emitterCallback)->
//                Call(isolate->GetCurrentContext()->Global(), ARRAY_SIZE(argv), argv);
//    }


    void WiiV8::New(const FunctionCallbackInfo <Value> &args) {
        Isolate *isolate = args.GetIsolate();

        HandleScope handleScope(isolate);

        if (args.IsConstructCall()) {
            // Invoked as constructor: `new WiiV8(...)`
            WiiV8 *obj = new WiiV8();
            obj->Wrap(args.This());
            args.GetReturnValue().Set(args.This());
        } else {
            // Invoked as plain function `WiiV8(...)`, turn into construct call.
            const int argc = 1;
            Local <Value> argv[argc] = {args[0]};
            Local <Function> cons = Local<Function>::New(isolate, constructor);
            Local <Context> context = isolate->GetCurrentContext();
            Local <Object> instance =
                    cons->NewInstance(context, argc, argv).ToLocalChecked();
            args.GetReturnValue().Set(instance);
        }
    }

    void WiiV8::Hello(const FunctionCallbackInfo <Value> &args) {
        Isolate *isolate = args.GetIsolate();
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, "world"));
    }

}