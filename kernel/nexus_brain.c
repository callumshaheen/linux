/*
 * =====================================================================================
 *
 *       Filename:  nexus_brain.c
 *
 *    Description:  The core logic for the Nexus Intent-Based OS. This file
 *                  contains the main syscall entry point and dispatcher.
 *
 *        Version:  1.4.1 (Phase 2.5 Corrected)
 *        Created:  [Current Date]
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Shaheen Nazir
 *   Organization:  Project Nexus
 *
 * =====================================================================================
 */

 #include <linux/kernel.h>
 #include <linux/module.h>
 #include <linux/syscalls.h>
 #include <linux/printk.h>
 #include <linux/uaccess.h>
 #include <linux/errno.h>
 #include <linux/sysinfo.h>
 #include <linux/sched/signal.h>
 #include <linux/fs.h>
 #include <linux/namei.h>
 #include <linux/dcache.h>
 #include <linux/mount.h>
 #include <linux/kmod.h>     // Required for call_usermodehelper
 #include <linux/slab.h>     // Required for kmalloc/kfree
 
 #include <linux/nexus_api.h>
 
 MODULE_LICENSE("GPL");
 MODULE_AUTHOR("Shaheen Nazir");
 MODULE_DESCRIPTION("Nexus OS Brain: Intent-Based Syscall Handler");
 MODULE_VERSION("1.4.1");
 
 /*
  * -------------------------------------------------------------------------------------
  *  PATHFINDER HANDLERS
  * -------------------------------------------------------------------------------------
  */
 
 static long handle_get_system_info(const struct nki_goal *goal)
 {
     long ret = 0;
     switch (goal->params.get_info.target_subsystem) {
     case SUBSYS_MEMORY: {
         struct sysinfo mem_info;
         void __user *user_buf = (void __user *)goal->params.get_info.output_buffer;
         if (goal->params.get_info.buffer_size < sizeof(struct sysinfo)) return -EINVAL;
         si_meminfo(&mem_info);
         if (copy_to_user(user_buf, &mem_info, sizeof(struct sysinfo))) return -EFAULT;
         break;
     }
     case SUBSYS_PROCESSES: {
         long process_count = 0;
         void __user *user_buf = (void __user *)goal->params.get_info.output_buffer;
         
         // --- THE FIX IS HERE ---
         // We must declare local variables to be used as iterators for the macro.
         struct task_struct *p;
         struct task_struct *t;
 
         if (goal->params.get_info.buffer_size < sizeof(long)) return -EINVAL;
         
         rcu_read_lock();
         // Use our local variables 'p' and 't' in the macro, not 'current'.
         for_each_process_thread(p, t) {
             process_count++;
         }
         rcu_read_unlock();
         
         if (copy_to_user(user_buf, &process_count, sizeof(long))) return -EFAULT;
         break;
     }
     default: ret = -ENOSYS; break;
     }
     return ret;
 }
 
 static long handle_configure_subsystem(const struct nki_goal *goal)
 {
     long ret = 0;
     switch (goal->params.configure_subsystem.target_subsystem) {
     case SUBSYS_BRIGHTNESS: {
         s64 value = goal->params.configure_subsystem.value;
         char s_value[12];
         struct file *filp;
         loff_t pos = 0;
         const char *path = "/sys/class/backlight/intel_backlight/brightness";
         if (value < 0 || value > 100) return -EINVAL;
         snprintf(s_value, sizeof(s_value), "%lld\n", value);
         filp = filp_open(path, O_WRONLY, 0);
         if (IS_ERR(filp)) return PTR_ERR(filp);
         ret = kernel_write(filp, s_value, strlen(s_value), &pos);
         filp_close(filp, NULL);
         ret = (ret < 0) ? ret : 0;
         break;
     }
     default: ret = -ENOSYS; break;
     }
     return ret;
 }
 
 static long handle_manage_files(const struct nki_goal *goal)
 {
     long ret = 0;
     char kpath[NKI_MAX_PATH_LEN];
     strncpy(kpath, goal->params.manage_files.path1, NKI_MAX_PATH_LEN);
     kpath[NKI_MAX_PATH_LEN - 1] = '\0';
 
     switch (goal->params.manage_files.operation) {
     case FILE_OP_CREATE: {
         struct file *filp = filp_open(kpath, O_CREAT | O_WRONLY, goal->params.manage_files.mode & 0777);
         if (IS_ERR(filp)) return PTR_ERR(filp);
         filp_close(filp, NULL);
         ret = 0;
         break;
     }
     case FILE_OP_DELETE: {
         struct path path;
         struct inode *dir_inode = NULL;
         ret = kern_path(kpath, LOOKUP_FOLLOW, &path);
         if (ret) return ret;
         if (d_is_dir(path.dentry)) {
             path_put(&path);
             return -EISDIR;
         }
         dir_inode = d_inode(path.dentry->d_parent);
         inode_lock(dir_inode);
         ret = vfs_unlink(mnt_idmap(path.mnt), dir_inode, path.dentry, NULL);
         inode_unlock(dir_inode);
         path_put(&path);
         break;
     }
     default: ret = -EINVAL; break;
     }
     return ret;
 }
 
 static long handle_manage_application(const struct nki_goal *goal)
 {
     char *argv[4];
     char *envp[] = { "HOME=/", "TERM=linux", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };
     int ret;
 
     printk(KERN_INFO "NEXUS_BRAIN: Pathfinder initiated for GOAL_MANAGE_APPLICATION.\n");
 
     switch(goal->params.manage_app.operation) {
     case APP_OP_START:
         printk(KERN_INFO "NEXUS_BRAIN: App Start op on path: %s\n", goal->params.manage_app.path);
 
         argv[0] = (char *)goal->params.manage_app.path;
         if (goal->params.manage_app.args[0] != '\0') {
             argv[1] = (char *)goal->params.manage_app.args;
             argv[2] = NULL;
         } else {
             argv[1] = NULL;
         }
 
         ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
 
         if (ret == 0) {
             printk(KERN_INFO "NEXUS_BRAIN: call_usermodehelper successful for %s.\n", argv[0]);
         } else {
             pr_err("NEXUS_BRAIN: call_usermodehelper failed for %s with error %d\n", argv[0], ret);
         }
         return ret;
 
     default:
         pr_warn("NEXUS_BRAIN: Unknown app operation requested: %d\n", goal->params.manage_app.operation);
         return -EINVAL;
     }
 }
 
 
 /*
  * -------------------------------------------------------------------------------------
  *  MAIN SYSTEM CALL ENTRY POINT
  * -------------------------------------------------------------------------------------
  */
 SYSCALL_DEFINE1(nexus_submit_goal, const struct nki_goal __user *, user_goal_ptr)
 {
     struct nki_goal *goal;
     long ret = 0;
 
     goal = kmalloc(sizeof(*goal), GFP_KERNEL);
     if (!goal)
         return -ENOMEM;
 
     if (copy_from_user(goal, user_goal_ptr, sizeof(*goal))) {
         kfree(goal);
         return -EFAULT;
     }
 
     goal->params.manage_files.path1[NKI_MAX_PATH_LEN - 1] = '\0';
     goal->params.manage_files.path2[NKI_MAX_PATH_LEN - 1] = '\0';
     goal->params.manage_app.path[NKI_MAX_PATH_LEN - 1] = '\0';
     goal->params.manage_app.args[NKI_MAX_ARGS_LEN - 1] = '\0';
 
     pr_info("NEXUS_BRAIN: Syscall initiated. Dispatching goal_id: %d\n", goal->goal_id);
 
     switch (goal->goal_id) {
     case GOAL_GET_SYSTEM_INFO:
         ret = handle_get_system_info(goal);
         break;
     case GOAL_CONFIGURE_SUBSYSTEM:
         ret = handle_configure_subsystem(goal);
         break;
     case GOAL_MANAGE_FILES:
         ret = handle_manage_files(goal);
         break;
     case GOAL_MANAGE_APPLICATION:
         ret = handle_manage_application(goal);
         break;
     default:
         pr_warn("NEXUS_BRAIN: EINVAL - Received unknown goal_id: %d\n", goal->goal_id);
         ret = -EINVAL;
         break;
     }
 
     kfree(goal);
     return ret;
 }