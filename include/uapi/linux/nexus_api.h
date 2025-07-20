/*
 * =====================================================================================
 *
 *       Filename:  nexus_api.h
 *
 *    Description:  The Master UAPI Header for the Intent-Based Nexus Kernel Interface.
 *                  This file defines the contract between user-space and the kernel.
 *
 *        Version:  1.3 (Phase 2.5 Implemented)
 *        Created:  [Current Date]
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Shaheen Nazir
 *   Organization:  Project Nexus
 *
 * =====================================================================================
 */

 #ifndef _UAPI_LINUX_NEXUS_API_H
 #define _UAPI_LINUX_NEXUS_API_H
 
 // Guard for C++ compilers, ensuring proper linkage
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #include <linux/types.h>
 
 /*
  * -------------------------------------------------------------------------------------
  *  CORE ENUMS AND DEFINITIONS
  * -------------------------------------------------------------------------------------
  */
 
 /**
  * enum nki_goal_id - Defines all high-level goals a user-space application can request.
  */
 enum nki_goal_id {
     GOAL_UNSPECIFIED         = 0,
     GOAL_GET_SYSTEM_INFO     = 1,
     GOAL_CONFIGURE_SUBSYSTEM = 2,
     GOAL_MANAGE_FILES        = 3,
     GOAL_MANAGE_APPLICATION  = 4, // NEW in 2.5
 };
 
 /**
  * enum nki_subsystem_id - Defines specific kernel subsystems that can be targets for goals.
  */
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
 
 /**
  * enum nki_file_op - Defines the file operation to be performed for GOAL_MANAGE_FILES.
  */
 enum nki_file_op {
     FILE_OP_UNSPECIFIED = 0,
     FILE_OP_CREATE      = 1,
     FILE_OP_DELETE      = 2,
     FILE_OP_RENAME      = 3, // Reserved for future use
 };
 
 /**
  * enum nki_app_op - Defines the application operation to be performed.
  */
 enum nki_app_op {
     APP_OP_UNSPECIFIED = 0,
     APP_OP_START       = 1,
     APP_OP_TERMINATE   = 2, // Reserved for future use
 };
 
 // Define maximum lengths to prevent buffer overflows in the kernel.
 #define NKI_MAX_PATH_LEN 256
 #define NKI_MAX_ARGS_LEN 512
 
 /*
  * -------------------------------------------------------------------------------------
  *  PARAMETER STRUCTURES
  * -------------------------------------------------------------------------------------
  */
 
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
     char             path1[NKI_MAX_PATH_LEN];
     char             path2[NKI_MAX_PATH_LEN];
     __u32            mode;
 };
 
 /**
  * struct nki_params_manage_application - Parameters for GOAL_MANAGE_APPLICATION
  * @operation: The action to take (e.g., start).
  * @path: The absolute path to the executable.
  * @args: A string of command-line arguments for the application.
  */
 struct nki_params_manage_application {
     enum nki_app_op operation;
     char            path[NKI_MAX_PATH_LEN];
     char            args[NKI_MAX_ARGS_LEN];
 };
 
 
 /*
  * -------------------------------------------------------------------------------------
  *  MASTER GOAL STRUCTURE
  * -------------------------------------------------------------------------------------
  */
 
 /**
  * struct nki_goal - The single, master structure passed from user space to the kernel
  *                   for every nexus_submit_goal syscall.
  */
 struct nki_goal {
     enum nki_goal_id goal_id;
     union {
         struct nki_params_get_info              get_info;
         struct nki_params_configure_subsystem   configure_subsystem;
         struct nki_params_manage_files          manage_files;
         struct nki_params_manage_application    manage_app; // NEW in 2.5
     } params;
 };
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* _UAPI_LINUX_NEXUS_API_H */