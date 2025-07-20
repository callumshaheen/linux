/*
 * CFFI-safe version of the Nexus API header (v2)
 * Contains only pure C declarations for python parsing.
 * All #include and #define directives have been removed.
 */

/* CORE ENUMS */
enum nki_goal_id {
    GOAL_UNSPECIFIED         = 0,
    GOAL_GET_SYSTEM_INFO     = 1,
    GOAL_CONFIGURE_SUBSYSTEM = 2,
    GOAL_MANAGE_FILES        = 3,
    GOAL_MANAGE_APPLICATION  = 4,
};

enum nki_subsystem_id {
    SUBSYS_UNSPECIFIED = 0,
    SUBSYS_KERNEL      = 1,
    SUBSYS_MEMORY      = 2,
    SUBSYS_CPU         = 3,
    SUBSYS_PROCESSES   = 4,
    SUBSYS_FILESYSTEM  = 5,
    SUBSYS_NETWORK     = 6,
    SUBSYS_BRIGHTNESS  = 7,
};

enum nki_file_op {
    FILE_OP_UNSPECIFIED = 0,
    FILE_OP_CREATE      = 1,
    FILE_OP_DELETE      = 2,
    FILE_OP_RENAME      = 3,
};

enum nki_app_op {
    APP_OP_UNSPECIFIED = 0,
    APP_OP_START       = 1,
    APP_OP_TERMINATE   = 2,
};

/* PARAMETER STRUCTURES */
struct nki_params_get_info {
    enum nki_subsystem_id target_subsystem;
    __u64                 output_buffer;
    __u32                 buffer_size;
};

struct nki_params_configure_subsystem {
    enum nki_subsystem_id target_subsystem;
    __s64                 value;
};

struct nki_params_manage_files {
    enum nki_file_op operation;
    char             path1[256]; /* NKI_MAX_PATH_LEN */
    char             path2[256]; /* NKI_MAX_PATH_LEN */
    __u32            mode;
};

struct nki_params_manage_application {
    enum nki_app_op operation;
    char            path[256]; /* NKI_MAX_PATH_LEN */
    char            args[512]; /* NKI_MAX_ARGS_LEN */
};

/* MASTER GOAL STRUCTURE */
struct nki_goal {
    enum nki_goal_id goal_id;
    union {
        struct nki_params_get_info              get_info;
        struct nki_params_configure_subsystem   configure_subsystem;
        struct nki_params_manage_files          manage_files;
        struct nki_params_manage_application    manage_app;
    } params;
};