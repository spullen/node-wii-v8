// myobject.h
#ifndef MYOBJECT_H
#define MYOBJECT_H

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>

#include <uv.h>
#include <bluetooth/bluetooth.h>
#include <cwiid.h>

using namespace v8;
using namespace node;

namespace WiiV8 {

    class WiiV8 : public node::ObjectWrap {
    public:
        static void Init(v8::Isolate *isolate);
        static void NewInstance(const v8::FunctionCallbackInfo <v8::Value> &args);

        static v8::Persistent<v8::String> ir_event;
        static v8::Persistent<v8::String> acc_event;
        static v8::Persistent<v8::String> nunclassic_event;
        static v8::Persistent<v8::String> balance_event;
        static v8::Persistent<v8::String> motionplus_event;
        static v8::Persistent<v8::String> button_event;
        static v8::Persistent<v8::String> error_event;
        static v8::Persistent<v8::String> status_event;

        static v8::Persistent<v8::FunctionTemplate> constructor_template;

        /**
         * Function: Connect
         *   Accepts an address and creates a connection.
         *
         * Returns:
         *   a string explaining the error code.
         */
        int Connect(bdaddr_t * mac);
        int Disconnect();

        int Rumble(bool on);
        int Led(int index, bool on);
        int RequestStatus();
        int Reporting(int mode, bool on);

    protected:
        WiiV8();
        virtual ~WiiV8();

        static void Connect(const v8::FunctionCallbackInfo <v8::Value> &args);
        static void UV_Connect(uv_work_t* req);
        static void UV_AfterConnect(uv_work_t* req, int status);

        static void Disconnect(const v8::FunctionCallbackInfo <v8::Value> &args);

        // Callback from libcwiid's thread
        static void HandleMessages(cwiid_wiimote_t *, int, union cwiid_mesg [], struct timespec *);

        // Callback from Nodejs's thread which calls one of the Handle*Message methods
        static void HandleMessagesAfter(uv_work_t *req, int status);

        // The following methods parse and emit events
        void HandleAccMessage    (Isolate* isolate, struct timespec *ts, cwiid_acc_mesg * msg);
        void HandleButtonMessage (Isolate* isolate, struct timespec *ts, cwiid_btn_mesg * msg);
        void HandleErrorMessage  (Isolate* isolate, struct timespec *ts, cwiid_error_mesg * msg);
        void HandleNunchukMessage(Isolate* isolate, struct timespec *ts, cwiid_nunchuk_mesg * msg);
        void HandleClassicMessage(Isolate* isolate, struct timespec *ts, cwiid_classic_mesg * msg);
        void HandleBalanceMessage(Isolate* isolate, struct timespec *ts, cwiid_balance_mesg * msg);
        void HandleMotionPlusMessage(Isolate* isolate, struct timespec *ts, cwiid_motionplus_mesg * msg);
        void HandleIRMessage     (Isolate* isolate, struct timespec *ts, cwiid_ir_mesg * msg);
        void HandleStatusMessage (Isolate* isolate, struct timespec *ts, cwiid_status_mesg * msg);

        // The following methods turn things on and off
        static void Rumble(const v8::FunctionCallbackInfo <v8::Value> &args);
        static void Led(const v8::FunctionCallbackInfo <v8::Value> &args);
        static void RequestStatus(const v8::FunctionCallbackInfo <v8::Value> &args);
        static void IrReporting(const v8::FunctionCallbackInfo <v8::Value> &args);
        static void AccReporting(const v8::FunctionCallbackInfo <v8::Value> &args);
        static void ExtReporting(const v8::FunctionCallbackInfo <v8::Value> &args);
        static void ButtonReporting(const v8::FunctionCallbackInfo <v8::Value> &args);

        static void SetEmitter(const v8::FunctionCallbackInfo <v8::Value> &args);

    private:
        static Persistent <Function> emitterCallback;

        // The v8 instance for this object
        v8::Handle <v8::Object> self;

        // Pointer to a WiiV8 handle
        cwiid_wiimote_t *wiimote;

        //struct representing a WiiV8 state
        struct cwiid_state state;

        // bluetooth address value
        bdaddr_t mac;

        // button identifier
        int button;

        struct connect_request {
            WiiV8 *wiiV8;
            bdaddr_t mac;
            int err;
            Persistent<Function> callback;
        };

        // Passes a WiiV8 event from libcwiid's thread to the Nodejs's thread
        struct message_request {
            WiiV8 *wiimote;
            struct timespec timestamp;
            int len;
            union cwiid_mesg mesgs[1]; // Variable size array containing len elements
        };

        static void New(const v8::FunctionCallbackInfo <v8::Value> &args);

        static void Hello(const v8::FunctionCallbackInfo <v8::Value> &args);

        static v8::Persistent <v8::Function> constructor;

//        void Emit(Isolate* isolate, Local<Value> argv);
    };

}

#endif