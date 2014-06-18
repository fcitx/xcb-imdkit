typedef struct _ximattr_fr
{
    uint16_t attribute_ID;
    uint16_t type_of_the_value;
    uint16_t length_of_im_attribute;
    bytearray im_attribute;
} ximattr_fr;

typedef struct _xicattr_fr
{
    uint16_t attribute_ID;
    uint16_t type_of_the_value;
    uint16_t length_of_ic_attribute;
    bytearray ic_attribute;
} xicattr_fr;

typedef struct _ximattribute_fr
{
    uint16_t attribute_ID;
    uint16_t value_length;
    bytearray value;
} ximattribute_fr;

typedef struct _xicattribute_fr
{
    uint16_t attribute_ID;
    uint16_t value_length;
    bytearray value;
} xicattribute_fr;

typedef struct _ximtriggerkey_fr
{
    uint32_t keysym;
    uint32_t modifier;
    uint32_t modifier_mask;
} ximtriggerkey_fr;

typedef struct _encodinginfo_fr
{
    uint16_t length_of_encoding_info;
    bytearray encoding_info;
} encodinginfo_fr;

typedef struct _str_fr
{
    uint8_t length_of_string;
    bytearray string;
} str_fr;

typedef struct _xpcs_fr
{
    uint16_t length_of_string_in_bytes;
    bytearray string;
} xpcs_fr;

typedef struct _ext_fr
{
    uint16_t extension_major_opcode;
    uint16_t extension_minor_opcode;
    uint16_t length_of_extension_name;
    bytearray extension_name;
} ext_fr;

typedef struct _inputstyle_fr
{
    uint32_t inputstyle;
} inputstyle_fr;

typedef struct _attr_head_fr
{
    uint16_t attribute_id;
    uint16_t attribute_length;
} attr_head_fr;

typedef struct _short_fr
{
    uint16_t value;
} short_fr;

typedef struct _long_fr
{
    uint32_t value;
} long_fr;

typedef struct _xrectangle_fr
{
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} xrectangle_fr;

typedef struct _xpoint_fr
{
    uint16_t x;
    uint16_t y;
} xpoint_fr;

typedef struct _fontset_fr
{
    uint16_t length_of_base_font_name;
    bytearray base_font_name_list;
} fontset_fr;

typedef struct _input_styles_fr
{
    struct {
        uint32_t size;
        inputstyle_fr* items;
    } XIMStyle_list;
} input_styles_fr;

typedef struct _packet_header_fr
{
    uint8_t major_opcode;
    uint8_t minor_opcode;
    uint16_t length;
} packet_header_fr;

typedef struct _error_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    uint16_t flag;
    uint16_t Error_Code;
    uint16_t length_of_error_detail;
    uint16_t type_of_error_detail;
    bytearray error_detail;
} error_fr;

typedef struct _connect_fr
{
    uint8_t byte_order;
    uint16_t client_major_protocol_version;
    uint16_t client_minor_protocol_version;
    struct {
        uint32_t size;
        xpcs_fr* items;
    } client_auth_protocol_names;
} connect_fr;

typedef struct _connect_reply_fr
{
    uint16_t server_major_protocol_version;
    uint16_t server_minor_protocol_version;
} connect_reply_fr;

typedef struct _auth_required_fr
{
    uint8_t auth_protocol_index;
    uint8_t auth_data1;
    bytearray auth_data2;
} auth_required_fr;

typedef struct _auth_reply_fr
{
    uint8_t field0;
    bytearray field1;
} auth_reply_fr;

typedef struct _auth_next_fr
{
    uint8_t auth_data1;
    bytearray auth_data2;
} auth_next_fr;

typedef struct _auth_setup_fr
{
    struct {
        uint32_t size;
        xpcs_fr* items;
    } server_auth_protocol_names;
} auth_setup_fr;

typedef struct _auth_ng_fr
{
} auth_ng_fr;

typedef struct _disconnect_fr
{
} disconnect_fr;

typedef struct _disconnect_reply_fr
{
} disconnect_reply_fr;

typedef struct _open_fr
{
    str_fr field0;
} open_fr;

typedef struct _open_reply_fr
{
    uint16_t input_method_ID;
    struct {
        uint32_t size;
        ximattr_fr* items;
    } IM_attribute_supported;
    struct {
        uint32_t size;
        xicattr_fr* items;
    } IC_attribute_supported;
} open_reply_fr;

typedef struct _close_fr
{
    uint16_t input_method_ID;
} close_fr;

typedef struct _close_reply_fr
{
    uint16_t input_method_ID;
} close_reply_fr;

typedef struct _register_triggerkeys_fr
{
    uint16_t input_method_ID;
    struct {
        uint32_t size;
        ximtriggerkey_fr* items;
    } on_keys_list;
    struct {
        uint32_t size;
        ximtriggerkey_fr* items;
    } off_keys_list;
} register_triggerkeys_fr;

typedef struct _trigger_notify_fr
{
    uint16_t input_mehotd_ID;
    uint16_t input_context_ID;
    uint32_t flag;
    uint32_t index_of_keys_list;
    uint32_t client_select_event_mask;
} trigger_notify_fr;

typedef struct _trigger_notify_reply_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
} trigger_notify_reply_fr;

typedef struct _set_event_mask_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    uint32_t forward_event_mask;
    uint32_t synchronous_event_mask;
} set_event_mask_fr;

typedef struct _encoding_negotiation_fr
{
    uint16_t input_method_ID;
    struct {
        uint32_t size;
        str_fr* items;
    } supported_list_of_encoding_in_IM_library;
    struct {
        uint32_t size;
        encodinginfo_fr* items;
    } list_of_encodings_supported_in_th;
} encoding_negotiation_fr;

typedef struct _encoding_negotiation_reply_fr
{
    uint16_t input_method_ID;
    uint16_t category_of_the_encoding_determined;
    uint16_t index_of_the_encoding_dterminated;
} encoding_negotiation_reply_fr;

typedef struct _query_extension_fr
{
    uint16_t input_method_ID;
    struct {
        uint32_t size;
        str_fr* items;
    } extensions_supported_by_the_IM_library;
} query_extension_fr;

typedef struct _query_extension_reply_fr
{
    uint16_t input_method_ID;
    struct {
        uint32_t size;
        ext_fr* items;
    } list_of_extensions_supported_by_th;
} query_extension_reply_fr;

typedef struct _get_im_values_fr
{
    uint16_t input_method_ID;
    struct {
        uint32_t size;
        uint16_t* items;
    } im_attribute_id;
} get_im_values_fr;

typedef struct _get_im_values_reply_fr
{
    uint16_t input_method_ID;
    struct {
        uint32_t size;
        ximattribute_fr* items;
    } im_attribute_returned;
} get_im_values_reply_fr;

typedef struct _create_ic_fr
{
    uint16_t input_method_ID;
    struct {
        uint32_t size;
        xicattribute_fr* items;
    } ic_attributes;
} create_ic_fr;

typedef struct _create_ic_reply_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
} create_ic_reply_fr;

typedef struct _destroy_ic_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
} destroy_ic_fr;

typedef struct _destroy_ic_reply_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
} destroy_ic_reply_fr;

typedef struct _set_ic_values_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    struct {
        uint32_t size;
        xicattribute_fr* items;
    } ic_attribute;
} set_ic_values_fr;

typedef struct _set_ic_values_reply_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
} set_ic_values_reply_fr;

typedef struct _get_ic_values_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    struct {
        uint32_t size;
        uint16_t* items;
    } ic_attribute;
} get_ic_values_fr;

typedef struct _get_ic_values_reply_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    struct {
        uint32_t size;
        xicattribute_fr* items;
    } ic_attribute;
} get_ic_values_reply_fr;

typedef struct _set_ic_focus_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
} set_ic_focus_fr;

typedef struct _unset_ic_focus_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
} unset_ic_focus_fr;

typedef struct _forward_event_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    uint16_t flag;
    uint16_t sequence_number;
} forward_event_fr;

typedef struct _sync_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
} sync_fr;

typedef struct _sync_reply_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
} sync_reply_fr;

typedef struct _commit_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    uint16_t flag;
    uint16_t byte_length_of_committed_string;
    bytearray committed_string;
    struct {
        uint32_t size;
        uint32_t* items;
    } keysym;
} commit_fr;

typedef struct _commit_chars_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    uint16_t flag;
    uint16_t byte_length_of_committed_string;
    bytearray committed_string;
} commit_chars_fr;

typedef struct _commit_both_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    uint16_t flag;
    uint32_t keysym;
    uint16_t byte_length_of_committed_string;
    bytearray committed_string;
} commit_both_fr;

typedef struct _reset_ic_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
} reset_ic_fr;

typedef struct _reset_ic_reply_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    uint16_t byte_length_of_committed_string;
    bytearray committed_string;
} reset_ic_reply_fr;

typedef struct _geometry_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
} geometry_fr;

typedef struct _str_conversion_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    uint32_t XIMStringConversionPosition;
    uint32_t XIMStringConversionType;
    uint32_t XIMStringConversionOperation;
    uint16_t length_to_multiply_th;
    uint16_t length_of_the_string_to_b;
    bytearray string;
} str_conversion_fr;

typedef struct _str_conversion_reply_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    uint32_t XIMStringConversionFeedback;
    uint16_t length_of_the_retrieved_string;
    bytearray retrieved_string;
    struct {
        uint32_t size;
        uint32_t* items;
    } feedback_array;
} str_conversion_reply_fr;

typedef struct _preedit_start_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
} preedit_start_fr;

typedef struct _preedit_start_reply_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    uint32_t return_value;
} preedit_start_reply_fr;

typedef struct _preedit_draw_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    uint32_t caret;
    uint32_t chg_first;
    uint32_t chg_length;
    uint32_t status;
    uint16_t length_of_preedit_string;
    bytearray preedit_string;
    struct {
        uint32_t size;
        uint32_t* items;
    } feedback_array;
} preedit_draw_fr;

typedef struct _preedit_caret_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    uint32_t position;
    uint32_t direction;
    uint32_t style;
} preedit_caret_fr;

typedef struct _preedit_caret_reply_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    uint32_t position;
} preedit_caret_reply_fr;

typedef struct _preedit_done_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
} preedit_done_fr;

typedef struct _status_start_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
} status_start_fr;

typedef struct _status_draw_text_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    uint32_t type;
    uint32_t status;
    uint16_t length_of_status_string;
    bytearray status_string;
    struct {
        uint32_t size;
        uint32_t* items;
    } feedback_array;
} status_draw_text_fr;

typedef struct _status_draw_bitmap_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    uint32_t type;
    uint32_t pixmap_data;
} status_draw_bitmap_fr;

typedef struct _status_done_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
} status_done_fr;

typedef struct _ext_set_event_mask_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    uint32_t filter_event_mask;
    uint32_t intercept_event_mask;
    uint32_t select_event_mask;
    uint32_t forward_event_mask;
    uint32_t synchronous_event_mask;
} ext_set_event_mask_fr;

typedef struct _ext_forward_keyevent_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    uint16_t flag;
    uint16_t sequence_number;
    uint8_t xEvent_u_u_type;
    uint8_t keycode;
    uint16_t state;
    uint32_t time;
    uint32_t window;
} ext_forward_keyevent_fr;

typedef struct _ext_move_fr
{
    uint16_t input_method_ID;
    uint16_t input_context_ID;
    uint16_t X;
    uint16_t Y;
} ext_move_fr;


void ximattr_fr_read(ximattr_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* ximattr_fr_write(ximattr_fr *frame, uint8_t *data, bool swap);
size_t ximattr_fr_size(ximattr_fr *frame);
void ximattr_fr_free(ximattr_fr *frame);

void xicattr_fr_read(xicattr_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* xicattr_fr_write(xicattr_fr *frame, uint8_t *data, bool swap);
size_t xicattr_fr_size(xicattr_fr *frame);
void xicattr_fr_free(xicattr_fr *frame);

void ximattribute_fr_read(ximattribute_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* ximattribute_fr_write(ximattribute_fr *frame, uint8_t *data, bool swap);
size_t ximattribute_fr_size(ximattribute_fr *frame);
void ximattribute_fr_free(ximattribute_fr *frame);

void xicattribute_fr_read(xicattribute_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* xicattribute_fr_write(xicattribute_fr *frame, uint8_t *data, bool swap);
size_t xicattribute_fr_size(xicattribute_fr *frame);
void xicattribute_fr_free(xicattribute_fr *frame);

void ximtriggerkey_fr_read(ximtriggerkey_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* ximtriggerkey_fr_write(ximtriggerkey_fr *frame, uint8_t *data, bool swap);
size_t ximtriggerkey_fr_size(ximtriggerkey_fr *frame);
void ximtriggerkey_fr_free(ximtriggerkey_fr *frame);

void encodinginfo_fr_read(encodinginfo_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* encodinginfo_fr_write(encodinginfo_fr *frame, uint8_t *data, bool swap);
size_t encodinginfo_fr_size(encodinginfo_fr *frame);
void encodinginfo_fr_free(encodinginfo_fr *frame);

void str_fr_read(str_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* str_fr_write(str_fr *frame, uint8_t *data, bool swap);
size_t str_fr_size(str_fr *frame);
void str_fr_free(str_fr *frame);

void xpcs_fr_read(xpcs_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* xpcs_fr_write(xpcs_fr *frame, uint8_t *data, bool swap);
size_t xpcs_fr_size(xpcs_fr *frame);
void xpcs_fr_free(xpcs_fr *frame);

void ext_fr_read(ext_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* ext_fr_write(ext_fr *frame, uint8_t *data, bool swap);
size_t ext_fr_size(ext_fr *frame);
void ext_fr_free(ext_fr *frame);

void inputstyle_fr_read(inputstyle_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* inputstyle_fr_write(inputstyle_fr *frame, uint8_t *data, bool swap);
size_t inputstyle_fr_size(inputstyle_fr *frame);
void inputstyle_fr_free(inputstyle_fr *frame);

void attr_head_fr_read(attr_head_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* attr_head_fr_write(attr_head_fr *frame, uint8_t *data, bool swap);
size_t attr_head_fr_size(attr_head_fr *frame);
void attr_head_fr_free(attr_head_fr *frame);

void short_fr_read(short_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* short_fr_write(short_fr *frame, uint8_t *data, bool swap);
size_t short_fr_size(short_fr *frame);
void short_fr_free(short_fr *frame);

void long_fr_read(long_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* long_fr_write(long_fr *frame, uint8_t *data, bool swap);
size_t long_fr_size(long_fr *frame);
void long_fr_free(long_fr *frame);

void xrectangle_fr_read(xrectangle_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* xrectangle_fr_write(xrectangle_fr *frame, uint8_t *data, bool swap);
size_t xrectangle_fr_size(xrectangle_fr *frame);
void xrectangle_fr_free(xrectangle_fr *frame);

void xpoint_fr_read(xpoint_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* xpoint_fr_write(xpoint_fr *frame, uint8_t *data, bool swap);
size_t xpoint_fr_size(xpoint_fr *frame);
void xpoint_fr_free(xpoint_fr *frame);

void fontset_fr_read(fontset_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* fontset_fr_write(fontset_fr *frame, uint8_t *data, bool swap);
size_t fontset_fr_size(fontset_fr *frame);
void fontset_fr_free(fontset_fr *frame);

void input_styles_fr_read(input_styles_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* input_styles_fr_write(input_styles_fr *frame, uint8_t *data, bool swap);
size_t input_styles_fr_size(input_styles_fr *frame);
void input_styles_fr_free(input_styles_fr *frame);

void packet_header_fr_read(packet_header_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* packet_header_fr_write(packet_header_fr *frame, uint8_t *data, bool swap);
size_t packet_header_fr_size(packet_header_fr *frame);
void packet_header_fr_free(packet_header_fr *frame);

void error_fr_read(error_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* error_fr_write(error_fr *frame, uint8_t *data, bool swap);
size_t error_fr_size(error_fr *frame);
void error_fr_free(error_fr *frame);

void connect_fr_read(connect_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* connect_fr_write(connect_fr *frame, uint8_t *data, bool swap);
size_t connect_fr_size(connect_fr *frame);
void connect_fr_free(connect_fr *frame);

void connect_reply_fr_read(connect_reply_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* connect_reply_fr_write(connect_reply_fr *frame, uint8_t *data, bool swap);
size_t connect_reply_fr_size(connect_reply_fr *frame);
void connect_reply_fr_free(connect_reply_fr *frame);

void auth_required_fr_read(auth_required_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* auth_required_fr_write(auth_required_fr *frame, uint8_t *data, bool swap);
size_t auth_required_fr_size(auth_required_fr *frame);
void auth_required_fr_free(auth_required_fr *frame);

void auth_reply_fr_read(auth_reply_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* auth_reply_fr_write(auth_reply_fr *frame, uint8_t *data, bool swap);
size_t auth_reply_fr_size(auth_reply_fr *frame);
void auth_reply_fr_free(auth_reply_fr *frame);

void auth_next_fr_read(auth_next_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* auth_next_fr_write(auth_next_fr *frame, uint8_t *data, bool swap);
size_t auth_next_fr_size(auth_next_fr *frame);
void auth_next_fr_free(auth_next_fr *frame);

void auth_setup_fr_read(auth_setup_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* auth_setup_fr_write(auth_setup_fr *frame, uint8_t *data, bool swap);
size_t auth_setup_fr_size(auth_setup_fr *frame);
void auth_setup_fr_free(auth_setup_fr *frame);

void auth_ng_fr_read(auth_ng_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* auth_ng_fr_write(auth_ng_fr *frame, uint8_t *data, bool swap);
size_t auth_ng_fr_size(auth_ng_fr *frame);
void auth_ng_fr_free(auth_ng_fr *frame);

void disconnect_fr_read(disconnect_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* disconnect_fr_write(disconnect_fr *frame, uint8_t *data, bool swap);
size_t disconnect_fr_size(disconnect_fr *frame);
void disconnect_fr_free(disconnect_fr *frame);

void disconnect_reply_fr_read(disconnect_reply_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* disconnect_reply_fr_write(disconnect_reply_fr *frame, uint8_t *data, bool swap);
size_t disconnect_reply_fr_size(disconnect_reply_fr *frame);
void disconnect_reply_fr_free(disconnect_reply_fr *frame);

void open_fr_read(open_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* open_fr_write(open_fr *frame, uint8_t *data, bool swap);
size_t open_fr_size(open_fr *frame);
void open_fr_free(open_fr *frame);

void open_reply_fr_read(open_reply_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* open_reply_fr_write(open_reply_fr *frame, uint8_t *data, bool swap);
size_t open_reply_fr_size(open_reply_fr *frame);
void open_reply_fr_free(open_reply_fr *frame);

void close_fr_read(close_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* close_fr_write(close_fr *frame, uint8_t *data, bool swap);
size_t close_fr_size(close_fr *frame);
void close_fr_free(close_fr *frame);

void close_reply_fr_read(close_reply_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* close_reply_fr_write(close_reply_fr *frame, uint8_t *data, bool swap);
size_t close_reply_fr_size(close_reply_fr *frame);
void close_reply_fr_free(close_reply_fr *frame);

void register_triggerkeys_fr_read(register_triggerkeys_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* register_triggerkeys_fr_write(register_triggerkeys_fr *frame, uint8_t *data, bool swap);
size_t register_triggerkeys_fr_size(register_triggerkeys_fr *frame);
void register_triggerkeys_fr_free(register_triggerkeys_fr *frame);

void trigger_notify_fr_read(trigger_notify_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* trigger_notify_fr_write(trigger_notify_fr *frame, uint8_t *data, bool swap);
size_t trigger_notify_fr_size(trigger_notify_fr *frame);
void trigger_notify_fr_free(trigger_notify_fr *frame);

void trigger_notify_reply_fr_read(trigger_notify_reply_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* trigger_notify_reply_fr_write(trigger_notify_reply_fr *frame, uint8_t *data, bool swap);
size_t trigger_notify_reply_fr_size(trigger_notify_reply_fr *frame);
void trigger_notify_reply_fr_free(trigger_notify_reply_fr *frame);

void set_event_mask_fr_read(set_event_mask_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* set_event_mask_fr_write(set_event_mask_fr *frame, uint8_t *data, bool swap);
size_t set_event_mask_fr_size(set_event_mask_fr *frame);
void set_event_mask_fr_free(set_event_mask_fr *frame);

void encoding_negotiation_fr_read(encoding_negotiation_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* encoding_negotiation_fr_write(encoding_negotiation_fr *frame, uint8_t *data, bool swap);
size_t encoding_negotiation_fr_size(encoding_negotiation_fr *frame);
void encoding_negotiation_fr_free(encoding_negotiation_fr *frame);

void encoding_negotiation_reply_fr_read(encoding_negotiation_reply_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* encoding_negotiation_reply_fr_write(encoding_negotiation_reply_fr *frame, uint8_t *data, bool swap);
size_t encoding_negotiation_reply_fr_size(encoding_negotiation_reply_fr *frame);
void encoding_negotiation_reply_fr_free(encoding_negotiation_reply_fr *frame);

void query_extension_fr_read(query_extension_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* query_extension_fr_write(query_extension_fr *frame, uint8_t *data, bool swap);
size_t query_extension_fr_size(query_extension_fr *frame);
void query_extension_fr_free(query_extension_fr *frame);

void query_extension_reply_fr_read(query_extension_reply_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* query_extension_reply_fr_write(query_extension_reply_fr *frame, uint8_t *data, bool swap);
size_t query_extension_reply_fr_size(query_extension_reply_fr *frame);
void query_extension_reply_fr_free(query_extension_reply_fr *frame);

void get_im_values_fr_read(get_im_values_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* get_im_values_fr_write(get_im_values_fr *frame, uint8_t *data, bool swap);
size_t get_im_values_fr_size(get_im_values_fr *frame);
void get_im_values_fr_free(get_im_values_fr *frame);

void get_im_values_reply_fr_read(get_im_values_reply_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* get_im_values_reply_fr_write(get_im_values_reply_fr *frame, uint8_t *data, bool swap);
size_t get_im_values_reply_fr_size(get_im_values_reply_fr *frame);
void get_im_values_reply_fr_free(get_im_values_reply_fr *frame);

void create_ic_fr_read(create_ic_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* create_ic_fr_write(create_ic_fr *frame, uint8_t *data, bool swap);
size_t create_ic_fr_size(create_ic_fr *frame);
void create_ic_fr_free(create_ic_fr *frame);

void create_ic_reply_fr_read(create_ic_reply_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* create_ic_reply_fr_write(create_ic_reply_fr *frame, uint8_t *data, bool swap);
size_t create_ic_reply_fr_size(create_ic_reply_fr *frame);
void create_ic_reply_fr_free(create_ic_reply_fr *frame);

void destroy_ic_fr_read(destroy_ic_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* destroy_ic_fr_write(destroy_ic_fr *frame, uint8_t *data, bool swap);
size_t destroy_ic_fr_size(destroy_ic_fr *frame);
void destroy_ic_fr_free(destroy_ic_fr *frame);

void destroy_ic_reply_fr_read(destroy_ic_reply_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* destroy_ic_reply_fr_write(destroy_ic_reply_fr *frame, uint8_t *data, bool swap);
size_t destroy_ic_reply_fr_size(destroy_ic_reply_fr *frame);
void destroy_ic_reply_fr_free(destroy_ic_reply_fr *frame);

void set_ic_values_fr_read(set_ic_values_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* set_ic_values_fr_write(set_ic_values_fr *frame, uint8_t *data, bool swap);
size_t set_ic_values_fr_size(set_ic_values_fr *frame);
void set_ic_values_fr_free(set_ic_values_fr *frame);

void set_ic_values_reply_fr_read(set_ic_values_reply_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* set_ic_values_reply_fr_write(set_ic_values_reply_fr *frame, uint8_t *data, bool swap);
size_t set_ic_values_reply_fr_size(set_ic_values_reply_fr *frame);
void set_ic_values_reply_fr_free(set_ic_values_reply_fr *frame);

void get_ic_values_fr_read(get_ic_values_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* get_ic_values_fr_write(get_ic_values_fr *frame, uint8_t *data, bool swap);
size_t get_ic_values_fr_size(get_ic_values_fr *frame);
void get_ic_values_fr_free(get_ic_values_fr *frame);

void get_ic_values_reply_fr_read(get_ic_values_reply_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* get_ic_values_reply_fr_write(get_ic_values_reply_fr *frame, uint8_t *data, bool swap);
size_t get_ic_values_reply_fr_size(get_ic_values_reply_fr *frame);
void get_ic_values_reply_fr_free(get_ic_values_reply_fr *frame);

void set_ic_focus_fr_read(set_ic_focus_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* set_ic_focus_fr_write(set_ic_focus_fr *frame, uint8_t *data, bool swap);
size_t set_ic_focus_fr_size(set_ic_focus_fr *frame);
void set_ic_focus_fr_free(set_ic_focus_fr *frame);

void unset_ic_focus_fr_read(unset_ic_focus_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* unset_ic_focus_fr_write(unset_ic_focus_fr *frame, uint8_t *data, bool swap);
size_t unset_ic_focus_fr_size(unset_ic_focus_fr *frame);
void unset_ic_focus_fr_free(unset_ic_focus_fr *frame);

void forward_event_fr_read(forward_event_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* forward_event_fr_write(forward_event_fr *frame, uint8_t *data, bool swap);
size_t forward_event_fr_size(forward_event_fr *frame);
void forward_event_fr_free(forward_event_fr *frame);

void sync_fr_read(sync_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* sync_fr_write(sync_fr *frame, uint8_t *data, bool swap);
size_t sync_fr_size(sync_fr *frame);
void sync_fr_free(sync_fr *frame);

void sync_reply_fr_read(sync_reply_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* sync_reply_fr_write(sync_reply_fr *frame, uint8_t *data, bool swap);
size_t sync_reply_fr_size(sync_reply_fr *frame);
void sync_reply_fr_free(sync_reply_fr *frame);

void commit_fr_read(commit_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* commit_fr_write(commit_fr *frame, uint8_t *data, bool swap);
size_t commit_fr_size(commit_fr *frame);
void commit_fr_free(commit_fr *frame);

void commit_chars_fr_read(commit_chars_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* commit_chars_fr_write(commit_chars_fr *frame, uint8_t *data, bool swap);
size_t commit_chars_fr_size(commit_chars_fr *frame);
void commit_chars_fr_free(commit_chars_fr *frame);

void commit_both_fr_read(commit_both_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* commit_both_fr_write(commit_both_fr *frame, uint8_t *data, bool swap);
size_t commit_both_fr_size(commit_both_fr *frame);
void commit_both_fr_free(commit_both_fr *frame);

void reset_ic_fr_read(reset_ic_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* reset_ic_fr_write(reset_ic_fr *frame, uint8_t *data, bool swap);
size_t reset_ic_fr_size(reset_ic_fr *frame);
void reset_ic_fr_free(reset_ic_fr *frame);

void reset_ic_reply_fr_read(reset_ic_reply_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* reset_ic_reply_fr_write(reset_ic_reply_fr *frame, uint8_t *data, bool swap);
size_t reset_ic_reply_fr_size(reset_ic_reply_fr *frame);
void reset_ic_reply_fr_free(reset_ic_reply_fr *frame);

void geometry_fr_read(geometry_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* geometry_fr_write(geometry_fr *frame, uint8_t *data, bool swap);
size_t geometry_fr_size(geometry_fr *frame);
void geometry_fr_free(geometry_fr *frame);

void str_conversion_fr_read(str_conversion_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* str_conversion_fr_write(str_conversion_fr *frame, uint8_t *data, bool swap);
size_t str_conversion_fr_size(str_conversion_fr *frame);
void str_conversion_fr_free(str_conversion_fr *frame);

void str_conversion_reply_fr_read(str_conversion_reply_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* str_conversion_reply_fr_write(str_conversion_reply_fr *frame, uint8_t *data, bool swap);
size_t str_conversion_reply_fr_size(str_conversion_reply_fr *frame);
void str_conversion_reply_fr_free(str_conversion_reply_fr *frame);

void preedit_start_fr_read(preedit_start_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* preedit_start_fr_write(preedit_start_fr *frame, uint8_t *data, bool swap);
size_t preedit_start_fr_size(preedit_start_fr *frame);
void preedit_start_fr_free(preedit_start_fr *frame);

void preedit_start_reply_fr_read(preedit_start_reply_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* preedit_start_reply_fr_write(preedit_start_reply_fr *frame, uint8_t *data, bool swap);
size_t preedit_start_reply_fr_size(preedit_start_reply_fr *frame);
void preedit_start_reply_fr_free(preedit_start_reply_fr *frame);

void preedit_draw_fr_read(preedit_draw_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* preedit_draw_fr_write(preedit_draw_fr *frame, uint8_t *data, bool swap);
size_t preedit_draw_fr_size(preedit_draw_fr *frame);
void preedit_draw_fr_free(preedit_draw_fr *frame);

void preedit_caret_fr_read(preedit_caret_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* preedit_caret_fr_write(preedit_caret_fr *frame, uint8_t *data, bool swap);
size_t preedit_caret_fr_size(preedit_caret_fr *frame);
void preedit_caret_fr_free(preedit_caret_fr *frame);

void preedit_caret_reply_fr_read(preedit_caret_reply_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* preedit_caret_reply_fr_write(preedit_caret_reply_fr *frame, uint8_t *data, bool swap);
size_t preedit_caret_reply_fr_size(preedit_caret_reply_fr *frame);
void preedit_caret_reply_fr_free(preedit_caret_reply_fr *frame);

void preedit_done_fr_read(preedit_done_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* preedit_done_fr_write(preedit_done_fr *frame, uint8_t *data, bool swap);
size_t preedit_done_fr_size(preedit_done_fr *frame);
void preedit_done_fr_free(preedit_done_fr *frame);

void status_start_fr_read(status_start_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* status_start_fr_write(status_start_fr *frame, uint8_t *data, bool swap);
size_t status_start_fr_size(status_start_fr *frame);
void status_start_fr_free(status_start_fr *frame);

void status_draw_text_fr_read(status_draw_text_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* status_draw_text_fr_write(status_draw_text_fr *frame, uint8_t *data, bool swap);
size_t status_draw_text_fr_size(status_draw_text_fr *frame);
void status_draw_text_fr_free(status_draw_text_fr *frame);

void status_draw_bitmap_fr_read(status_draw_bitmap_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* status_draw_bitmap_fr_write(status_draw_bitmap_fr *frame, uint8_t *data, bool swap);
size_t status_draw_bitmap_fr_size(status_draw_bitmap_fr *frame);
void status_draw_bitmap_fr_free(status_draw_bitmap_fr *frame);

void status_done_fr_read(status_done_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* status_done_fr_write(status_done_fr *frame, uint8_t *data, bool swap);
size_t status_done_fr_size(status_done_fr *frame);
void status_done_fr_free(status_done_fr *frame);

void ext_set_event_mask_fr_read(ext_set_event_mask_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* ext_set_event_mask_fr_write(ext_set_event_mask_fr *frame, uint8_t *data, bool swap);
size_t ext_set_event_mask_fr_size(ext_set_event_mask_fr *frame);
void ext_set_event_mask_fr_free(ext_set_event_mask_fr *frame);

void ext_forward_keyevent_fr_read(ext_forward_keyevent_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* ext_forward_keyevent_fr_write(ext_forward_keyevent_fr *frame, uint8_t *data, bool swap);
size_t ext_forward_keyevent_fr_size(ext_forward_keyevent_fr *frame);
void ext_forward_keyevent_fr_free(ext_forward_keyevent_fr *frame);

void ext_move_fr_read(ext_move_fr *frame, uint8_t **data, size_t *len, bool swap);
uint8_t* ext_move_fr_write(ext_move_fr *frame, uint8_t *data, bool swap);
size_t ext_move_fr_size(ext_move_fr *frame);
void ext_move_fr_free(ext_move_fr *frame);

