savedcmd_sys_driver.mod := printf '%s\n'   sys_driver.o | awk '!x[$$0]++ { print("./"$$0) }' > sys_driver.mod
