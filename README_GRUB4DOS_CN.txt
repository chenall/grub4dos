本文档来源:http://bbs.znpc.net/viewthread.php?tid=5587
感谢zw2312914提供
原版是中英文对照的版本，这里已经经过删改。
需要看原版的可以从上面的贴子下载.

一些旧的网址已经删除,只保留目前在用的.

注:本文仅供参考,可能有一些更新没有在这里说明.如果有发现错误可以到论坛发贴或email联系我(grub4dos@chenall.net)

其它命令用法请参考
	http://chenall.net 或 http://grub4dos.chenall.net 处的 GRUB4DOS 有关文档。

外部命令的用法请参考
	http://chenall.net/post/tag/grub4dos/

项目主页 
	https://github.com/chenall/grub4dos

下载网址:
	http://grub4dos.chenall.net

工具和外部命令:
	http://code.google.com/p/grubutils/downloads/list

获取最新源代码的方法：

	git clone git://github.com/chenall/grub4dos.git
或
	svn co https://github.com/chenall/grub4dos grub4dos-src

通过你的 web 浏览器在线查看源代码：
	https://github.com/chenall/grub4dos


论坛（官方技术支持站点）：
	中文:
	http://bbs.wuyou.com/forumdisplay.php?fid=60
	http://bbs.znpc.net/forumdisplay.php?fid=4
	英文:
	http://reboot.pro/forum/66/
******************************************************************************
***                                GCC 黑名单                              ***
******************************************************************************

已知 gcc-4.6.x 所产生的程序在某些机器上无法正常运作。
来源: http://bbs.wuyou.net/viewthread.php?tid=274070

已知 gcc-4.7.x 所产生的程序无法运行 memtest86。
来源: http://bbs.wuyou.net/redirect.php?tid=180142&pid=2669810&goto=findpost#pid2669810

---------------------近期更新记录-----------------------------------------------
最新更新记录请查看,ChangeLog_chenall.txt
2011-01-03
	添加了GRUB4DOS的变量用法及相关介绍。

2011-01-02
	1.为了方便pause命令添加--test-key参数，显示按键扫描码.


2010-12-31 更新:
	1.pause命令增强，增加了显示按键扫描码的功能。需要debug 为-1.
	例子：显示一个按键扫描码
	debug -1 && pause && debug 1
	注：此功能已为被参数--test-key代替。

	2.hiddenmenu命令增强。增加一个--chkpass参数
		功能：	在隐藏菜单的时候按Esc键要输入正确的密码才可以显示菜单。
		注意：	1.该功能启用之后，如果按了其它按键则直接启动默认菜单。
			  默认的Esc按键可以自由设置。使用--chkpass=KEY
			2.菜单初始化有password命令时才需要输入密码。
			3.按键代码如果不清楚可以使用上面的功能来获取。
	例子：	hiddenmenu --chkpass=0x8500	按F11键才可以显示菜单。
		hiddenmenu --chkpass	 	按Esc键才可以显示菜单。
		hiddenmenu --chkpass=0x2200	按Alt+G显示菜单。

------------------------------------------------------------------------------
GRUB.EXE 用法：
		GRUB [--bypass] [--time-out=T] [--hot-key=K] [--config-file=FILE]
                        旁路       暂停时间        热键         配置文件
		
		这里的 FILE ，例如，可以是 (hd0,0)/menu.lst
		
                在 CONFIG.SYS 中，其配置行类似于：
		
			install=c:\some\where\grub.exe --config-file=FILE
		
                如果没有使用选项，GRUB.EXE简单的使用(hd0,0)/menu.lst
		来作为配置文件，只要它存在的话。（注意！我们最终将默认的配
                置文件从 (hd0,0)/boot/grub/menu.lst 变更为 (hd0,0)/menu.lst）
                （更新于2006-12-23。参见更新3）
		
                这里的分区(hd0,0)可以是一个Windows分区或者是一个linux分区，
                或者是被 GRUB 支持的其他任意分区。


                这里的FILE只接受GRUB风格的文件名。DOS风格的文件名不被调用
                （很明显，我们应该使用GRUB风格的文件名，原因是比如在Linux
                ext2分区中不能使用DOS风格的文件名来访问文件 ）。
                （参见更新2）                

                更新：FILE 可以是菜单的内容。使用分号来分隔嵌入 FILE 中的命
                令。FILE 可以被一对双引号括起来，示例：                

			GRUB --config-file="root (hd0,0);chainloader +1"
                这条命令将启动(hd0,0)上的系统。

                另一个例子：
			GRUB --config-file="reboot"
                这条命令将令机器重启。

                再例如：
			GRUB --config-file="halt"
                这条命令将令机器关闭。

                如果选项--bypass 被指定，GRUB将在暂停时间截止后从DOS退出。
                选项'--time-out=T' 指定了以秒计时的暂停值。如果指定了--bypass
                则T的默认值为5 ，而--bypass选项没有被指定时默认值为0 。                

                默认的热键值是 0x3920 (即空格键)。如果此键被按下，GRUB将正常
                启动。如果其他键被按下，GRUB 将立即终止并返回到DOS。参见下面
                的“int16 键盘扫描码”

                每个选项最多只能被指定一次。

                更新 2：DOS风格的文件名已经被支持（由John Cobb先生提供此补丁）。
                如果 FILE 的起始两个字符是“ #@ ”,那么 FILE 中其余的部分将被视
                为一个DOS风格的文件名。例如:

			GRUB --config-file="#@c:\menu.lst"

	        DOS风格的文件只有起始的4kB被使用。此文件应当是一个未被压缩的文
                本文件。                

                注意：你也可以在SHELL或者在CONFIG.SYS的INSTALL配置行使用‘DOS
                风格文件直接访问’。但不要在DEVICE 配置行使用它。DOS文档中表述
                了DOS设备驱动不能使用'打开文件'的DOS调用。


                更新 3(2006-12-23): 作为默认，GRUB.EXE将按以下顺序查找它的配置文件：

                       （DOS风格文件） .\menu.lst, 当前目录下的MENU.LST。
                       （DOS风格文件） \menu.lst, 当前驱动器根目录下的MENU.LST
                       （GRUB风格文件） /menu.lst, 启动设备根目录下的MENU.LST

                默认的启动设备还是(hd0,0)。


--------------------------------------------------------

更新 1 ：       版本0.2.0 同时带来一个新的东西。GRUB for NTLDR ，它能够从
                Windows NT/2000/XP的启动菜单启动到GRUB。复制GRLDR到Windows 
                NT/2000/XP的C盘根目录，并在C:\BOOT.INI中加入这样一行：

			C:\GRLDR="Start GRUB"

                这样就完成了安装。GRLDR应该与 BOOT.INI及NTLDR 位于相同的目录。
                注意 BOOT.INI 通常是隐藏的因而你要看见它必须取消隐藏。如果GRLDR
                在一个NTFS分区，应该将它复制到一个非NTFS分区的根目录（并且menu.lst
                文件应当同样这样做）。假如GRLDR被压缩了，比如在NTFS分区中对它启
                用了压缩的情况下，它将不能运行。

               即使这个磁盘的驱动器号已经被Windows设备管理器改变为另外的不同
                于 C 的盘号 ，你仍然需要在 BOOT.INI 中使用盘符 C ，否则，NTLDR
                将查找不到 GRLDR 文件。

                也就表示，如果你从一个软盘上启动NTLDR，你也需要把A:\BOOT.INI
                中的 GRLDR 的所在行这样写：

			C:\GRLDR="Start GRUB"

                 而不能象下面这样使用盘符 A :

			A:\GRLDR="Start GRUB"

		（注意在 BOOT.INI 位于软盘 A 的这个例子中，标记“C:\GRLDR” 
		实际将访问 A:\GRLDR ）


更新 2：	GRUB for linux 也一起被引入到版本0.2.0。
		你可以使用使用linux的引导程序 KEXEC,LILO,SYSLINUX 或者别的
		GRUB来启动它。（GRUB4LIN 已经合并到 GRUB.EXE 中）

		从Linux中直接启动到GRUB ，使用这样一组命令：

			kexec -l grub.exe
			kexec -e

                经由GRUB启动到GRUB，使用如下的命令：

			kernel (hd0,0)/grub.exe
			boot

		经由LILO启动到GRUB，在lilo.conf中加入这样几行：

			image=/boot/grub.exe
			label=grub.exe

		经由syslinux启动到GRUB,在syslinux.cfg中加入这样几行：

			label grub.exe
				kernel grub.exe

		LOADLIN在引导GRUB.EXE时可能会遇到问题，原因是grub.exe需要一些
		未更改的原始的BIOS中断向量，但是DOS破坏了这些中断向量，而loadlin
		在将控制权交给grub.exe前并没有恢复它们。

		
更新 3：        从版本0.4.0开始，DOS下的GRUB支持内存驱动器。示例:

			# boot into a floppy image
			map --mem (hd0,0)/floppy.img (fd0)
			map --hook
			chainloader (fd0)+1
			rootnoverify (fd0)
			map --floppies=1
			boot

		由于镜像将被拷贝到一个内存区域，所以镜像本身可以是非连续的，
		甚至可以是被gzip压缩过的。

		另一个例子：
			map --mem=-2880 (hd0,0)/floppy.img (fd0)

		这个内存驱动器(fd0)将占用至少1440 KB的内存。
		这也对那些小于1440 KB的1.44M 软盘镜像有用。

		再例如：
			map --mem --read-only (hd0,0)/hd.img (hd1)

		这个内存驱动器成为了一个硬盘驱动器，并且只读。
		这表示你不能向这个(hd1)中写入数据。

		你可以同时使用多个内存驱动器和多个原来的基于磁盘虚拟的仿真驱动器。

		如果BIOS不支持中断int15/EAX=e820h，你将不能使用任何内存驱动器。

更新 4：        对于内存驱动器仿真，一个单一的分区镜像可以被转换为整个硬盘镜像来
		

			map --mem (hd0,7)/win98.img (hd0)
			map --hook
			chainloader (hd0)+1
			rootnoverify (hd0)
			map --harddrives=1
			boot

		这里的win98.img是一个头部不含主引导记录和分区表的分区镜像。
		GRUB for DOS 确实会为这个内存驱动器(hd0)建立一个MBR和分区表。

更新 5：        现在GRLDR可以作为一个非模拟模式的可启动光盘的启动映像文件来使
		用。Linux用户使用示例：

			mkdir iso_root
			cp grldr iso_root
			mkisofs -R -b grldr -no-emul-boot -boot-load-seg 0x1000 -o bootable.iso iso_root

		另一种选择是，grldr也可以用同样的方法作为stage2_eltorito来使用。
		-boot-info-table选项是被允许使用的，但你可以省略它：

			mkdir iso_root
			cp grldr iso_root
			mkisofs -R -b grldr -no-emul-boot -boot-load-size 4 -o grldr.iso iso_root

		也要注意上面的可启动iso镜像是使用-boot-load-seg 0xHHHH选项来建立的话，
		就必须令HHHH大于或者等于1000(十六进制)
		如果HHHH小于1000(十六进制),虚拟机 QEMU 会死机。这是QEMU的一个缺陷。一
		个grldr.iso镜像可以使用也可以不使用-boot-load-seg 0xHHHH选项来创建。

		menu.lst文件应该被放置在光盘的根目录。

更新 6：        中文专用版被建立在"chinese"子目录中。
	       （ 由Gandalf先生提供此补丁,2005-06-27）

		中文专用版同时内附了scdrom 功能。
		（更新：scdrom 从2006-07-20起已被删除）

更新 7：        增加了内存驱动器(md) 。就像(nd)代表网络驱动器 (cd) 代表光驱一样，
		新的驱动器(md)实现了将整个内存作为一个磁盘驱动器来访问。
		(md)只工作在支持BIOS中断int15/EAX=E820h的系统上。

		现在，cat命令已经有了一个十六进制转储的新选项：--hex 
		以及通过--locate=STRING 在文件中找查找字符串。

		典型示例：

			cat --hex (hd0)+1

		这将用十六进制的表格显示 MBR 扇区。

			cat --hex (md)+2

		这将显示你内存起始处的1 KB内容(实际，这就是实模式的IDT表),也是
		使用十六进制的转储表。                
		
			cat --hex (md)0x800+1

		这将显示你的扩展内存的第一个扇区。
		
			cat --hex (hd0,0)+1
		这将显示(hd0,0)分区的第一扇区。通常这个扇区包含操作系统的引导记录。

更新 8：        增加了随机存贮驱动器 (rd) 。(md) 设备访问内存是从物理地址 0 
		开始，而 (rd) 可以访问起始于任何基地址的内存。随机存贮器的基
		址与长度可以通过 map 命令指定。详情请使用 "help map" 命令。
		你甚至能够为(rd)驱动器指定一个BIOS驱动器号。比如：
		map --ram-drive=0xf0。默认的 (rd) 驱动器号是使用0x7F的软驱号。
		如果 (rd) 是硬盘驱动器镜像，你应该用大于或等于0x80的值来改变
		它的驱动器号。（但应防止使用0xffff，因为0xffff是预留给(md)设
		备的。）

		(rd)+1这个标志始终代表文件，它包含存储在(rd)中的所有字节。

更新 9：        直接启动 WinNT/2K/XP 的 NTLDR 和 Win9x/ME 的 IO.SYS, 以及
		FreeDOS 的 KERNEL.SYS. 例如：

			chainloader --edx=0xPPYY (hd0,0)/ntldr
			boot

			chainloader --edx=0xYY (hd0,0)/io.sys
			boot

			chainloader --ebx=0xYY (hd0,0)/kernel.sys
			boot

		十六进制的YY 指定了启动驱动器号，十六进制的PP指定了 NTLDR 的
		启动分区号。如果启动驱动器是软驱，PP应该是一个十六进制的值ff,
		即十进制的255.

		对于 Freedos 的 KERNEL.SYS , 选项--edx不能执行，请使用--ebx 。

		当文件位于它们平常的位置时，选项 --edx (--ebx)可以省略。但某些
		情况下，这些选项是必需的。
		例如，假设被调用的ntldr文件在ext2分区(hd2,8)中，而你又希望它认
		为(hd0,7)的 windows 分区是启动分区，那么--edx就是必需的：

			chainloader --edx=0x0780 (hd2,8)/ntldr

		对于DOS核心（即，IO.SYS和KERNEL.SYS）,启动分区号是没有意义的，
		因此你只需指定恰当的启动驱动器号YY（不过指定了启动分区号也是
		无害的）。

		上面的PPYY也可以在chainloader命令之后通过root和rootnoverify命
		令来指定。例如：

			chainloader (hd2,6)/kernel.sys
			rootnoverify (hd0)	<-------- YY=80
			boot

			chainloader (hd0,0)/ntldr
			rootnoverify (hd0,5)	<-------- YY=80, PP=05
			boot

		提示：CMLDR （the ComMan LoaDeR,它被用于加载Windows的故障
		恢复控制台）可以像NTLDR一样被良好的加载。

		Bean 成功地解压和启动了WinME的IO.SYS，感谢这个出色的工作。

更新 10：       isolinux.bin (版本 3.73) 可以被自2009-02-09 起建立的版本加载。

			chainloader (cd)/isolinux.bin

		isolinux.bin必须是存在于在真实或虚拟的光驱之中的。

更新 11：       Grub 传统版的 stage2 文件可以通过下面的方法加载：

			chainloader --force --load-segment=0 --load-offset=0x8000 --boot-cs=0 --boot-ip=0x8200 (...)/.../stage2

更新 12：	增加了 exFAT 分区启动。


更新 13：	支持 udf 格式光盘。支持 iso9600 Joliet 扩展格式光盘。并可以启动它。
		可以把 grldr_cd.bin 作为 cdrom 的引导代码。

--------------------------------------------------------
这里用一些示例来演示磁盘仿真命令的用法：

1.        仿真硬盘分区C:为软驱A:并从C:中启动win98：

		map --read-only (hd0,0)+1 (fd0)
		chainloader (hd0,0)+1
		rootnoverify (hd0)
		boot

	在上面的示例中，(hd0,0)是一个装有win98的C:盘 。当win98启动完成后，你会发
	现A:中包含了C:的所有文件，而且如果你删除A:中的文件，C:上的相应文件也会消
	失。

	在map命令行中，(hdm,n)+1式的写法被解释成代表整个(hdm,n)分区，而不仅仅是
	此分区的第一扇区。
 
2.	将硬盘分区C:仿真为软驱A:并从A:中启动win98：

		map --read-only (hd0,0)+1 (fd0)
		map --hook
		chainloader (fd0)+1
		rootnoverify (fd0)
		map --floppies=1
		boot

	在“map --hook”命令之后，仿真立即生效，即使是在GRUB的命令行模式中。       
	
	注意“chainloader (fd0)+1” 中的(fd0)是仿真后的虚拟软驱 A：，而不是真
	实的软盘（因为映射现在已经被挂载了）。       


3.	仿真镜像文件为软驱A:并从C:盘中启动win98:

		map --read-only (hd0,0)/floppy.img (fd0)
		chainloader (hd0,0)+1
		rootnoverify (hd0)
		map --floppies=1
		map --harddrives=1
		boot

4.	仿真硬盘分区为第一硬盘并从中启动DOS:

		map --read-only (hd2,6)+1 (hd0)
		map --hook
		chainloader (hd0,0)+1
		rootnoverify (hd0)
		map --harddrives=1
		boot

	在这个示例中，(hd2,6)+1代表BIOS序号为3的(hd2)硬盘中的，扩展的DOS逻辑分区。

	如果一个DOS分区被仿真为一个硬盘，GRUB for DOS将首先定位分区表，通常，
	是在DOS分区的开头63个扇区中。如果那里没有分区表，GRUB for DOS将拒绝仿真。        

5.	仿真一个镜像文件为第一硬盘然后从中启动DOS:

		map --read-only (hd0,0)/harddisk.img (hd0)
		chainloader --load-length=512 (hd0,0)/harddisk.img
		rootnoverify (hd0)
		map --harddrives=1
		boot

	如果一个镜像文件被仿真为硬盘，此镜像文件必须包含MBR。也就是说，
	HARDDISK.IMG的第一扇区必须包含被仿真的虚拟硬盘的分区表。        

注意：  BIOS数据区中的软盘和硬盘的总数在映射期间没有被改变。当主板上没有配置真
	实的软驱时，你通常应该专门使用诸如‘map --floppioes=’以及
	 map --harddrives=’来设置它们。如果不这样做，DOS可能会启动失败。

	‘map --status’可以报告出一些有价值的东西。同时要注意‘map --floppies=’
	和‘map --harddrives=’需要在没有执行映射前单独使用。

	版本0.4.2引入了一个新参数，memdisk_raw,用以模拟和内存驱动器类似的原生模式。
	如果BIOS不支持中断int15/87h,或者int18/87h的支持有缺陷，你应该在任何内
	存盘被使用前设置这个变量。这里是一个示例：       

		map --memdisk-raw=1
		map --mem (hd0,0)/floppy.img (fd0)
		map --hook
		chainloader (fd0)+1
		rootnoverify (fd0)
		boot

	如果你碰到内存驱动器故障而又没有使用map --memdisk-raw=1时,你应当用
	‘map --memdisk-raw=1’来尝试一次。       

	你一执行‘map --memdisk-raw=0’之后，就应该执行一次‘map --unhook’
	（如果需要的话在这之后再执行‘map --hook’）。

	更新：memdisk_raw 现在默认值为1 。如果你希望使用中断int15/87h来访问内存
	驱动器，你应当令‘map --memdisk-raw=0 ’。

--------------------------------------------------------
	
	任意大小的软盘或硬盘可以被 GRUB for DOS 0.2.0 版仿真。
	镜像文件必须是连续的，否则GRUB for DOS 将拒绝执行。
       ‘blocklist’命令可以列举一个文件的碎片或者分块。
	在GRUB提示符下输入“help map”可以获得简要的命令说明。

	这样的形式
		map ... (fd?)
	是一个软盘仿真，而下面的形式
		map ... (hd?)
	是一个硬盘仿真。

	使用硬盘仿真时，基于安全因素最好不要去启动Windows 。
	Windows甚至可能会破坏掉所有的数据和你硬盘上的所有资料!!!!!!!!      

	关于--mem的更新 ：当使用--mem时，甚至是在进入Windows的时候，它看
	来都相当安全。Win98可以正常运行内存驱动器。        
	Windows NT/2000/XP不能识别仿真的驱动器，不管是否使用了--mem选项。        

******************************************************************************
***   grldr可启动的软盘或硬盘分区的说明					   ***
******************************************************************************

1. Ext2/3/4 引导扇区/引导记录的布局 （用于载入grldr）

------------------------------------------------------------------------------
一个EXT2/3/4的卷可以由GRUB启动。复制grldr和可选的menu.lst文件到这个EXT2/3/4
卷的根目录，并按照grldr.pbr的第3至4扇区建立它的引导扇区。那么，这个EXT2/3/4卷就是GRUB可启动的。

更新：	bootlace.com是一个dos/linux下的工具，它可以把GRLDR的引导记录安装到一个
        EXT2/3/4卷的第一扇区。

更新：	可以直接复制引导代码到启动分区,引导代码会自动生成头部参数。

偏移    长度    说明
======	======	==============================================================
00h	2	近转移指令的机器码

02h	1	0x90

03h	8	文件系统ID。 “EXT2/3/4”

0Bh	2	每扇区字节数。必须为512 。

0Dh	1	每块扇区数。有效值是2, 4, 8, 16 和 32。
                
0Eh	2	每块字节数。有效值是0x400, 0x800, 0x1000, 0x2000 和 0x4000。

10h	4	在pointers-per-block块中的指针数，即一个二级间接块包含的块数。

		有效值是0x10000, 0x40000, 0x100000, 0x400000 和	0x1000000。

14h	4	每块指针数，即一个间接块包含的块数。

		有效值是0x100, 0x200, 0x400, 0x800, 0x1000 。

18h	2	每磁道的扇区数。

1Ah	2	磁头数/面数

1Ch	4	隐藏扇区数（它们位于引导扇区之前）

		也被成为是分区的起始扇区。

		对于软盘，它应当为0 。
                
20h	4	文件系统中的扇区总数（或者是分区的扇区总数）。

24h	1	启动设备的 BIOS 驱动器号码。

		实际此字节在读入时被忽略。引导代码将把DL的值写入到此字节中。
                BIOS或者调用程序应当在DL中设置磁盘号码。

		我们假定所有的BIOS在DL中能传递正确的磁盘号码。
                糟糕的BIOS不被支持!!

25h	1	此分区在启动驱动器上的分区号

		0, 1, 2, 3 是主分区 。4, 5, 6, ... 等等是扩展分区中的逻辑分区。
		0xff代表整个磁盘。所以对于软盘，其分区号码应当是0xff 。

26h	2	字节计数的索引节点大小。（注意!我们在此处为索引节点大小使用
                了以前被保留的一个字，即两个字节）

28h	4	每组的i节点数  

		通常，1.44M软盘只有一个组，并且总的i节点数是184。所以此值
                应为184或者更大。

2Ch	4	组描述符的块号码。

		对于1024字节的块有效值是2 ，否则是1 。

		这里的值等于（s_first_data_block + 1）。

30h	462	机器码部分。

1FEh	2	引导签名AA55h 。

200h	510	机器码部分。

3FEh	2	引导签名AA55h 。


2. FAT12/FAT16 引导扇区/引导记录的布局 （用于载入grldr）

------------------------------------------------------------------------------
一个FAT12/16的卷是GRUB可启动的。复制grldr和可选的menu.lst文件到这个FAT12/16 卷
的根目录，并按照grldr.pbr的第二扇区建立它的引导扇区。然后这个FAT12/16的卷就是GRUB可启动的。

更新:   bootlace.com 是一个dos/linux下的工具，而它能把GRLDR的引导记录安装到一个
        FAT12/16卷的引导扇区。

偏移    长度    说明

======	======	==============================================================
00h	2	近转移指令的机器码

02h	1	0x90

03h	8	OEM名称字符串 （对该磁盘进行格式化的操作系统的名称）。

0Bh	2	每扇区字节数。必须为512 。

0Dh	1	每簇的扇区数。有效值是1, 2, 4, 8, 16, 32, 64和128 。但是簇大小
		大于32K的情况不应发生。

0Eh	2	保留的扇区数（第一文件分配表之前的扇区数，包括引导扇区），通常是1。

10h	1	文件分配表数（几乎总是2）。

11h	2	根目录项的最大个数。

13h	2	扇区总数（仅用于小磁盘，如果磁盘太大此处被设置为0，而偏移 20h 处
		则替代它使用）。

15h	1	媒体描述符字节，现在该此节已经没有太大意义了（参见后面）。

16h	2	每个文件分配表的扇区数。

18h	2	每个磁道的扇区数

1Ah	2	磁头或面的总数。

1Ch	4	隐藏扇区数（位于引导扇区之前）。

		也被称为是分区的开始扇区。

		对于软盘，它应当为0 。

20h	4	大磁盘的扇区总数。

24h	1	引导设备的BIOS磁盘号。

		实际此字节在读入时被忽略。引导代码将把DL中的值写入此字节。
                BIOS或者调用程序应当在DL中设置磁盘号码。

		我们假定所有的BIOS在DL中能传递正确的磁盘号码。
                糟糕的BIOS不被支持!!

25h	1	启动驱动器上此文件系统的分区号码。

		更新：此字段被忽略。

26h	1	签名（必须是28h或者29h以便能被 NT 识别）

27h	4	卷的序列号。

2Bh	11	卷标签。

36h	8	文件系统ID。“FAT12”, “FAT16”。

3Eh	446	机器码部分。

1FCh	4	引导签名AA550000h 。（Win9x 使用4字节作为魔数值。）


3. FAT32 引导扇区/引导记录的布局 （用于载入grldr）

------------------------------------------------------------------------------
一个FAT32的卷是GRUB可启动的。复制grldr和可选的menu.lst文件到这个FAT32卷的根
目录，并按照grldr.pbr的第一扇区建立它的引导扇区。然后这个FAT32的卷就是GRUB可启动的。

更新:   bootlace.com 是一个dos及linux下的工具而它能把 GRLDR 的引导记录安装
        到一个FAT32卷的引导扇区。


偏移    长度    说明

======	======	==============================================================
00h	2	近转移指令的机器码。

02h	1	0x90

		更新 （2006-07-31）：尽管GRLDR不再使用 LBA 指示码这个字节，
                但Windows 98会使用它。通常这个字节在 CHS 模式中应当设置为0x90
               （软盘尤其如此）。如果此字节未被正确设置，Windows 98 将不能识别
                软盘或分区。这一问题由neiljoy先生报告。非常感谢!

03h	8	OEM名称字符串 （对该磁盘进行格式化的操作系统的名称）

0Bh	2	每扇区字节数。必须为512 。

0Dh	1	每簇的扇区数。有效值是1, 2, 4, 8, 16, 32, 64 和 128 。
                但是簇大小大于32K的情况不应发生。

0Eh	2	保留的扇区数（第一文件分配表之前的扇区数，包括引导扇区），
                通常是1 。

10h	1	文件分配表数（几乎总是2）。

11h	2	（根目录项的最大个数）必须为0 。

13h	2	（仅用于小磁盘的扇区总数）必须为0 。

15h	1	媒体描述符字节，现在该此节已经没有太大意义了（参见后面）。

16h	2	（每个文件分配表的扇区数）必须为0 。

18h	2	每个磁道的扇区数

1Ah	2	磁头或面的总数。

1Ch	4	隐藏扇区数（它们位于引导扇区之前）。

		也被称作是分区的起始扇区。

		对于软盘，它应当为0 。

20h	4	大磁盘的扇区总数。

24h	4	每个文件分配表的 FAT32 扇区数。

28h	2	如果第7位被清零，所有文件分配表将被更新，否则0-3位给出当前活
                动的文件分配表，所有其它位被保留。

2Ah	2	高字节是主修订号码，低字节是小修订号码，现在都是0 。

2Ch	4	根目录起始簇。

30h	2	文件系统信息扇区。

32h	2	如果非零它给出具有引导记录的备份扇区，通常是6。

34h	12	保留，设为0 。

40h	1	启动设备的 BIOS 驱动器号码。

		第一硬盘是80h，第一软盘是00h。

		实际此字节在读入时被忽略。引导代码将把DL中的值写入此字节。
                BIOS或者调用程序应当在DL中设置磁盘号码。

		我们假定所有的 BIOS 在DL中能传递正确的磁盘号码。
                糟糕的BIOS不被支持!!

41h	1	启动驱动器上此文件系统的分区号码。

		此字节在读入时被忽略。引导代码将分区号码写到此字节。
		见下述的偏移5Dh 。
		更新：此字段被忽略。

42h	1	签名（必须是 28h 或者 29h 以便能被 NT 识别）

43h	4	卷的序列号。

47h	11	卷标签。

52h	8	文件系统标识。“FAT32 ”。

5Ah	418	机器码部分。

1FCh	4	引导签名AA550000h 。（Win9x 使用4字节作为魔数值。）


4.NTFS 引导扇区/引导记录的布局 （用于载入grldr）

------------------------------------------------------------------------------
一个NTFS的卷是GRUB可启动的。复制grldr和可选的menu.lst文件到这个NTFS卷的根
目录，并按照grldr.pbr的第7至10扇区建立它的引导扇区。然后这个NTFS的卷就是GRUB可启动的。

更新:   bootlace.com是一个在dos/linux下的工具，而它能把GRLDR的引导记录安装到
        一个NTFS卷的开头4个扇区。

偏移    长度    说明

======	======	==============================================================
00h	2       近转移指令的机器码。
                
02h	1	0x90

03h	8	分区标识“NTFS”

0Bh	2	每扇区字节数。必须为512 。

0Dh	1	每簇的扇区数。有效值是1, 2, 4, 8, 16, 32, 64 和 128 。
                但是簇大小大于32K的情况不应发生。

0Eh	2	（保留的扇区数）未被使用。

10h	1	（文件分配表数）必须为 0 。

11h	2	（根目录项的最大个数）必须为0 。

13h	2	（仅用于小磁盘的扇区总数）必须为0 。

15h	1	媒体描述符字节，现在该此节已经没有太大意义了（参见后面）。

16h	2	（每个文件分配表的扇区数）必须为0 。

18h	2	每个磁道的扇区数。

1Ah	2	磁头/面的总数。

1Ch	4	隐藏扇区数（它们位于引导扇区之前）。

		也被称作是分区的起始扇区。

		对于软盘，它应当为0 。

20h	4	(大磁盘的扇区总数）必须为 0 。

24h	4	（每个文件分配表的 NTFS 扇区数）- 通常是 80 00 80 00 ，值
                为80 00 00 00将被看作是由Windows XP格式化为NTFS的USB拇指碟

28h	8	此卷的扇区号。

30h	8	$MFT的逻辑簇号。

38h	8	$MFTMirr的逻辑簇号。

40h	4	每个MFT记录的簇数。

44h	4	每个索引的簇数。

48h	8	卷序列号

50h	4	校验和，通常为 0 。

54h	424	机器码部分。

1FCh	4	引导签名AA550000h 。（Win9x 使用4字节作为魔数值。）

200h	1536	末尾 3 个扇区为机器码部分。



5. exFAT 引导扇区/引导记录的原始布局（用于载入grldr）
   exFAT 引导代码由 Fan JianYe 提供
------------------------------------------------------------------------------
一个exFAT的卷是GRUB可启动的。复制grldr和可选的menu.lst文件到这个exFAT卷的根
目录，并按照grldr.pbr的第5至6扇区建立它的引导扇区。然后这个exFAT的卷就是GRUB可启动的。

注意：如果直接复制引导代码到启动分区，需要从该分区启动一次，自动填充校验码。
否则 Windows 会认为该分区没有格式化。

偏移    长度    说明
======	======	==============================================================
00h	3	0xEB7690	跳转代码。  
03h	8	"EXFAT   "	签名


++++++++ The new increase ++++++++
0bh	2    	每扇区字节数
0dh	1    	每簇扇区数
0eh	4   	数据的起始绝对扇区
12h	4			当前簇在FAT表的绝对扇区
16h	2    	EIOS 支持  位7=1
18h	2    	每磁道扇区数
1ah	2    	磁头数
1ch	4   	隐含扇区数
20h	4   	保留
24h	1    	磁盘的物理驱动器号
25h	1    	分区号
26h	2    	保留
28h	4   	FAT表开始绝对扇区号
2ch	4   	保留
++++++++ The new increase ++++++++

30h	16	全部为“00”。
40h	8	分区起始扇区号。相当于"隐含扇区数" 	
48h	8	分区扇区数。相当于"磁盘逻辑总扇区"
50h	4	FAT表起始扇区号，通常为128号扇区，该值为相对于卷(分区)0扇区号而言。相当于"保留扇区数"
54h	4	FAT表扇区数。相当于"每文件分配表占扇区数"
58h	4	簇起始位置扇区号，该值用以描述文件系统中的第1个簇（即2号簇）的起始扇区号。
		通常2号簇分配给簇位图使用，因此，该值也就是簇位图的起始扇区号。		
5ch	4	卷内的总簇数。
60h	4	根目录起始簇号。相当于"磁盘根目录起始簇号"
64h	4	卷ID。相当于"磁盘卷的序列号"
68h	2	文件系统版本。   VV.MM（01.00此版本）
6ah	2	内容	  位偏移 	尺寸 	描述
              	活动FAT	     0  	1 	0/1=第一/第二。相当于"文件分配表镜像标志"
              	卷脏         1		1	0/1=清洁/肮脏。	
            	媒体失败     2 		1 	0/1=没有失败/故障报告
          	零     	     3 		1 	没有意义
              	保留         4 		c
6ch	1	每扇区字节数，表示方法为，以2的N次方表示。
6dh	1	每簇扇区数，以2的N次方表示。
6eh	1	FAT表的数。这个数字是1或2，2在TexFAT中使用。
6fh	1	驱动器号。INT 13 使用。
70h	1	簇(堆)在使用中的百分比
71h	7	保留
78h	390	引导代码.
1feh	2	引导签名AA55。
200h	510	引导代码。
3feh	2	引导签名AA55。


6. MBR中的FAT12/16/32/exFAT/EXT2 引导代码合并，占用2扇区多一点。

------------------------------------------------------------------------------       
   合并后的 FAT12/16/32/exFAT 当前 BPB 结构:
   grldr.pbr 中 exFAT 自建的 BPB 结构:

偏移	长度	说明
======	======	==============================================================
00h	2    	EB 2E   跳转代码
02h     1    	分区类型 / EIOS 标记
             	Bit 0   FAT12
           	Bit 1   FAT16
              	Bit 2   FAT32
             	Bit 3   exFAT
             	Bit 4   EXT2
             	Bit 5	保留
		Bit 6	EXT4 64位文件系统
             	Bit 7   EIOS
03h     4   	根扇区
07h     4   	主目录（FAT12/16）绝对起始扇区
0bh     2    	每扇区字节
0dh     1    	每簇扇区
0eh     4	数据的起始绝对扇区
12h     4 	当前簇在FAT表的绝对扇区
16h     2	"6412"  签名
18h     2    	每磁道扇区
1ah     2    	磁头数
1ch     4   	分区起始绝对扇区
20h     4 	磁盘逻辑总扇区
24h     1    	驱动器号
25h     1    	分区号
26h     2    	保留
28h     4   	FATs绝对起始扇区
2ch     4   	根目录起始簇号  


附录A：FAT32 的文件系统信息扇区（不用于引导grldr）

偏移    长度    说明
======	======	==============================================================
0h	4	起始处签名 41615252h 。

4h	480	被保留,设为 0 。

1E4h	4	故障征兆指数结构签名 61417272h 

1E8h	4	最后已知的空闲簇数，如果它等于FFFFFFFFh，簇数是未知的。

1ECh	4	假如它等于FFFFFFFFh的话，你应当去寻找一个空闲簇的簇号码 。
		此字段没有被设置。

1F0h	12	被保留,设为 0 。

1FCh	4	结尾签名AA550000h 。

------------------------------------------------------------------------------

附录B：媒体描述字节（非用于grldr）

此媒体描述字节没有意义，因为有些字节具有多重意义，例如 F0h 。

字节    磁盘种类        扇区    头      磁道    容量
----	------------	-------	-----	------	--------
FFh	5 1/4"		8	2	40	320KB
FEh	5 1/4"		8	1	40	160KB
FDh	5 1/4"		9	2	40	360KB
FCh	5 1/4"		9	1	40	180KB
FBh	both		9	2	80	640KB
FAh	both		9	1	80	320KB
F9h	5 1/4"		15	2	80	1200KB
F9h	3 1/2"		9	2	80	720KB
F0h	3 1/2"		18 	2	80	1440KB
F0h	3 1/2"		36 	2	80	2880KB
F8h	hard disk	NA	NA	NA	NA

******************************************************************************
***   grldr.mbr - 怎样将grldr.mbr写到硬盘的主引导磁道			   ***
******************************************************************************
  grldr.mbr包含能够用作主引导记录的代码。此代码负责搜索所有分区的grldr，并且在发现
  它后装载它。现在被支持的分区种类是：FAT12/16/32,NTFS,EXT2/3/4，EXFAT。在扩展分区
  上的逻辑分区也被支持，条件是此扩展分区与微软兼容。实际上，搜索机制没有充分地测试
  分区类型（0x85 ）的Linux 的扩展分区。

怎样将 GRLDR.MBR 写到硬盘的主引导磁道？

  首先，读入 Windows 磁盘签名及分区信息字节（总共72字节，从主引导记录的偏移
  0x01b8到0x01ff处），并且放置到GRLDR.MBR的开始扇区的相同范围的偏移0x01b8
  到0x01ff处。

  如果硬盘上的主引导记录是由微软的FDISK产生的单一的扇区主引导记录，
  可以选择把它复制到GRLDR.MBR的第二扇区。

  GRLDR.MBR的第二扇区叫作“原先的主引导记录”。当找不到GRLDR后，将从“原先的主引
  导记录”开始引导。

  不需要其它的步骤，当所有的上述的必要的改变已经完成后，现在只要将GRLDR.MBR写到
  主引导磁道。这就全部完成了。

注意：主引导磁道表示的是硬盘的第一条磁道。

注意：GRLDR.MBR 的自举代码只在分区的根目录寻找GRLDR。你最好把menu.lst文件和GRLDR
放置在一起。（就是说放在和 GRLDR 相同分区的相同根目录下。）

“grldr” 文件名在ext2分区中必须是小写字母，而且grldr的文件种类必须是纯普通文件。
其它种类，例如，符号链接文件也是不行的。

  更新： bootlace.com 是一个在 DOS/LINUX 下的能把 grldr.mbr 安装到主引导记录
  的工具。整个grldr.mbr被嵌入到bootlace.com工具内部，因此 bootlace.com可以独
  立使用。参见后面。

以下内容供开发人员参考.....
******************************************************************************
***               grldr.mbr - Details about the control bytes              ***
******************************************************************************
                  grldr.mbr - 其控制字节的详述

有六个字节可以控制GRLDR.MBR引导过程。

偏移量  长度    说明

======	======	==============================================================
5ah	1	  第 0 位＝1 ：禁止搜索软盘上的GRLDR 。
                  第 0 位＝0 ：允许搜索软盘上的GRLDR 。

		  第 1 位＝1 ：禁止引导分区表无效的原主引导记录
                            （通常是一个操作系统的引导扇区）
                  第 1 位＝0 ：允许引导分区表无效的原主引导记录
                            （通常是一个操作系统的引导扇区）

		  第 2 位 = 1 ：禁止无条件进入命令行 （见下面的`--duce'）
                  第 2 位 = 0 ：允许无条件进入命令行（见下面的`--duce'）

		  第 3 位= 1 ：禁止改变磁盘几何参数（见下面的 `--chs-no-tune'）
                  第 3 位= 0 ：允许改变磁盘几何参数（见下面的 `--chs-no-tune'）

                  第 4 位到第 6 位：被保留

                  第 7 位=1：在搜索 GRLDR 之后尝试引导原先的主引导记录
		  第 7 位=0：在搜索 GRLDR 之前尝试引导原先的主引导记录

5bh	1	等待键被按下时的暂停秒数。0xff代表始终暂停（即无休止的）。


5ch	2	热键代码。高字节是扫描码，低字节是ASCII码。默认值是0x3920，代
                表的是空格键。如果此键被按下，GRUB将在引导原先的主引导记录之前
		启动。见“ int16 键盘扫描码”。

5eh	1	优先引导的驱动器号，0xff 代表没有。

5fh	1	优先引导的分区号，0xff 代表整个驱动器。

		如果优先引导的驱动器号是0xff,搜索 GRLDR 的顺序是：

			(hd0,0), (hd0,1), ..., (hd0,L),(L=max partition number) 
			(hd1,0), (hd1,1), ..., (hd1,M),(M=max partition number)
			... ... ... ... ... ... ... ... 
			(hdX,0), (hdX,1), ..., (hdX,N),(N=max partition number)
						       (X=max harddrive number)
			(fd0)

		
                否则，如果优先引导的驱动器号假定为Y （且不等于0xff），而优先引导
                的分区号为K,那么搜索 GRLDR 的顺序和上面一样。

              
                注意：如果Y小于0x80 ,那么这个（Y）驱动器代表软驱，否则就是硬盘驱动器。
                      而（Y,K）代表Y号硬盘驱动器上的 k 号分区。

******************************************************************************
***        bootlace.com - 安装GRLDR.MBR自举代码到MBR                       ***
******************************************************************************

BOOTLACE.COM 能将GRLDR的引导记录安装到硬盘驱动器或硬盘映像文件的主引导记录中，
或者安装到软盘或者软盘映像的引导扇区。

用法：
	bootlace.com  选项  设备或文件
选项：
	--read-only		对指定的设备或文件执行所有操作，但是并不不真正地写入。

	--restore-mbr		恢复原先的主引导记录。

	--mbr-no-bpb		即使最靠前的是一个FAT分区，也不复制BPB表到主引导记录。

	--no-backup-mbr		不复制旧的主引导记录到设备或文件的第二扇区。

	--force-backup-mbr	强行复制主引导记录到设备或文件的第二扇区。

	--mbr-enable-floppy	允许搜索软盘上的GRLDR 。

	--mbr-disable-floppy	禁止搜索软盘上的GRLDR 。

	--mbr-enable-osbr	允许引导分区表无效的原先的主引导记录（通常是
                                操作系统的引导扇区）

	--mbr-disable-osbr	禁止引导分区表无效的原先的主引导记录（通常是
                                操作系统的引导扇区）

	--duce			禁止无条件进入命令行功能。

                                任何人按‘C’键都可以正常取得命令行控制台，绕
                                过了所有的配置文件。（包括预置的配置文件）。
                                这是一种安全漏洞。所以我们需要这一选项来禁止这
                                种情况。

                                DUCE 即 Disable Unconditional Command-line Entrance 
                                的缩写，意为无条件的（或不受控的）命令行入口。

	--chs-no-tune		禁止磁盘几何参数修正功能。

	--boot-prevmbr-first	在搜索GRLDR之前，先尝试引导原先的主引导记录。

	--boot-prevmbr-last	在搜索GRLDR之后，再尝试引导原先的主引导记录。

	--preferred-drive=D	优先引导的驱动器号，驱动器号D应大于0而小于255 。

	--preferred-partition=P	优先引导的分区号，分区号P应大于0 而小于255 。

	--serial-number=SN	为硬盘驱动器设置一个新的序列号码。SN必须是非零的数。

	--time-out=T		在引导原先的主引导记录前等待 T 秒钟。如果T是0xff，
				则始终等待。默认值是 5 。
	
	--hot-key=K		如果热键被按下，在引导原先的主引导记录之前启动
                                GRUB 。K 是一个双字节的值，如同 int16/AH=1 返回
                                到AX寄存器的值一样。高字节是扫描码，低字节是ASCII
                                码。默认值是 0x3920，即空格键。参见“int16键盘扫描码”。
                                
	--floppy		如果设备或文件是软盘，使用这一选项。

	--floppy=N		如果设备或文件是一个硬盘驱动器上的分区，使用此选项。
                                N用于指定分区号码：0 ，1，2 和 3 为主分区，而 4，5，
                                6，...等等为逻辑分区。

	--sectors-per-track=S	为--floppy 选项指定每磁道扇区数。S 应大于1且小于63,
				默认值是63 。

	--heads=H		为--floppy选项指定磁头数，H应大于1且小于256 。默认值是255 。

	--start-sector=B	为--floppy=N 选项指定隐藏扇区。

	--total-sectors=C	为--floppy 选项指定总扇区数。默认值是 0 。


	--install-partition=I	将引导记录安装到指定的硬盘驱动器或硬盘映像（设备或文件）
                                的第 I 号分区的引导区中。
																
	--gpt	安装 grldr.mbr 到 gpt 分区类型的设备。


DEVICE_OR_FILE： 设备或者映像文件的文件名。对于DOS，BIOS驱动器号（两位的十六进制
或三位的十进制数）可以被用来访问驱动器。BIOS驱动器号0表示第一软盘，1表示第二硬盘；
0x80 表示第一硬盘驱动器，0x81表示第二硬盘驱动器，等等。

注意：BOOTLACE.COM 仅仅是把引导代码写到MBR中。引导代码需要加载GRLDR作为GRUB启动
过程的第二（最后）阶段。因而在BOOTLACE.COM成功执行前或者是执行后，GRLDR应当被复
制到任一受支持分区的根目录下，当前受支持分区的文件系统类型仅有FAT12,FAT16,FAT32, 
NTFS,EXT2,EXT3,EXT4,EXFAT。

注意 2：如果DEVICE_OR_FILE是硬盘设备或是硬盘映像文件，它必须包含有效的分区表，
否则，BOOTLACE.COM 将安装失败。如果设备或文件是指向软驱或者软盘映像文件，那么
他必须包含一个受支持的文件系统（FAT12,FAT16,FAT32,NTFS,EXT2,EXT3,EXT4,EXFAT 等之一）。


重要!! 如果你安装grldr的引导代码到一个软盘或者一个分区，此软盘或分区将只能从
grldr引导 ，而你原本的IO.SYS（DOS/Win9x/Me）和NTLDR（WinNT/2K/XP）将变为不能
引导。这是由于软盘或分区的原始引导记录被覆盖了。而把GRLDR的引导记录安装到MBR
则没有这个问题。
更新：在最新版本的GRUB4DOS中NTLDR，IO.SYS或KERNEL.SYS等文件，可以被直接加载。

提示：如果文件名的开始是短划线(-)或数字，你可以在它前面加上目录名(./) 或 (.\)。

示例：
        在Linux下安装GRLDR的引导代码到MBR：
		bootlace.com  /dev/sda

        在DOS下安装GRLDR的引导代码到MBR：
		bootlace.com  0x80

        在DOS、Windows或Linux下安装GRLDR的引导代码到硬盘映像：
		bootlace.com  hd.img

        在Linux下安装GRLDR的引导代码到软盘：
		bootlace.com  --floppy /dev/fd0

        在DOS下安装GRLDR的引导代码到软盘：
		bootlace.com  --floppy 0x00

        在DOS、Windows或Linux下安装GRLDR的引导代码到软盘映像：
		bootlace.com  --floppy floppy.img

BOOTLACE.COM 无法在Windows NT/2000/XP/2003下正常运行。它被希望（和设计）用于
DOS/Win9x和Linux中。

更新：对于映像文件，在Windows NT/2000/XP/2003下，bootlace.com 可以正常使用。

bootlace.com不能在 Windows NT/2000/XP/2003 中运行的原因是，bootlace.com是一个DOS工具而 Windows NT/2000/XP/2003 
不认可它对设备的访问,你可以通过WINHEX/DISKRW之类的工具来间接的完全这个操作.
一个DISKRW的例子在这里:
	http://bbs.znpc.net/viewthread.php?tid=5447

******************************************************************************
***                  kexec-tools 应当打上1.101发布的补丁                   ***
******************************************************************************
           
kexec-tools-1.101-patch 是为kexec-tools-1.101发布的补丁。没有这个补丁Kexec 加载
grub.exe 会失败。

kexec-tools 的主页是:

	http://www.xmission.com/~ebiederm/files/kexec/

注意: 在使用 kexec 前应该使 Linux 核心支持 KEXEC 系统调用。

                           重要更新

现在不再需要`kexec-tools-1.101-patch'补丁而且它已经被删除了。很糟糕的是，执行
`kexec -l grub.exe --initrd=imgfile'竟会失败。所以请不要再使用它。

******************************************************************************
***                     从Linux直接转换到DOS/Win9x                         ***
******************************************************************************
              

使用kexec，我们能够轻易地从运行中的 Linux 系统启动到 DOS/Win9x 。

假如 WIN98.IMG 是一个可引导的硬盘映像，按照如下操作：

kexec -l grub.exe --initrd=WIN98.IMG --command-line="--config-file=map (rd) (hd0); map --hook; chainloader (hd0)+1; rootnoverify (hd0)"

kexec -e

如果DOS.IMG是一个可引导的软盘映像，按照以下方法：

kexec -l grub.exe --initrd=DOS.IMG --command-line="--config-file=map (rd) (fd0); map --hook; chainloader (fd0)+1; rootnoverify (fd0)"

kexec -e

注意，按照这种方式，我们可以不用使用真实的 DOS/Win9x 磁盘就启动到 DOS/Win9x 。
我们不需要FAT分区而只需要一个映像文件。

我们已经注意到通过使用 kexec 和 grub.exe，Linux本身就能够成为一个大的引导管理器。
这给安装程序或者引导程序或者初始化程序的开发者带来了方便。

当然，grub.exe和可引导的磁盘映像也能够被运行中的GRUB 或LILO 或syslinux 加载。例如：

1.通过 GRUB 加载：

	kernel (hd0,0)/grub.exe --config-file="map (rd) (fd0); map --hook; chainloader (fd0)+1; rootnoverify (fd0)"
	initrd (hd0,0)/DOS.IMG
	boot

2.通过 LILO 加载：

	image=/boot/grub.exe
		label=grub.exe
		initrd=/boot/DOS.IMG
		append="--config-file=map (rd) (fd0); map --hook; chainloader (fd0)+1; rootnoverify (fd0)"

3.通过 SYSLINUX 加载：

	label grub.exe
		kernel grub.exe
		append initrd=DOS.IMG --config-file="map (rd) (fd0); map --hook; chainloader (fd0)+1; rootnoverify (fd0)"

注意：如果使用上面的‘map (rd) (...)’失败，你可以使用‘map (rd)+1 (...)’代替，
然后再试一次。

******************************************************************************
***                      键盘 BIOS 扫描码和 ASCII 码表                     ***
******************************************************************************
                  

键盘 bios 扫描码和 ASCII 字符码表能够通过 web 网获取,例如，使用 google 查找
"3920 372A 4A2D 4E2B 352F"。这里有两项主要的结果：

1.    转自“http://heim.ifi.uio.no/~stanisls/helppc/scan_codes.html”:

INT 16 - 键盘扫描码

       键位      常态      上档态    控制态    变更态

	A	  1E61	    1E41      1E01	1E00
	B	  3062	    3042      3002	3000
	C	  2E63	    2E43      2E03	2E00
	D	  2064	    2044      2004	2000
	E	  1265	    1245      1205	1200
	F	  2166	    2146      2106	2100
	G	  2267	    2247      2207	2200
	H	  2368	    2348      2308	2300
	I	  1769	    1749      1709	1700
	J	  246A	    244A      240A	2400
	K	  256B	    254B      250B	2500
	L	  266C	    264C      260C	2600
	M	  326D	    324D      320D	3200
	N	  316E	    314E      310E	3100
	O	  186F	    184F      180F	1800
	P	  1970	    1950      1910	1900
	Q	  1071	    1051      1011	1000
	R	  1372	    1352      1312	1300
	S	  1F73	    1F53      1F13	1F00
	T	  1474	    1454      1414	1400
	U	  1675	    1655      1615	1600
	V	  2F76	    2F56      2F16	2F00
	W	  1177	    1157      1117	1100
	X	  2D78	    2D58      2D18	2D00
	Y	  1579	    1559      1519	1500
	Z	  2C7A	    2C5A      2C1A	2C00

       键位      常态      上档态    控制态    变更态

	1	  0231	    0221		7800
	2	  0332	    0340      0300	7900
	3	  0433	    0423		7A00
	4	  0534	    0524		7B00
	5	  0635	    0625		7C00
	6	  0736	    075E      071E	7D00
	7	  0837	    0826		7E00
	8	  0938	    092A		7F00
	9	  0A39	    0A28		8000
	0	  0B30	    0B29		8100

       键位      常态      上档态    控制态    变更态

	-	  0C2D	    0C5F      0C1F	8200
	=	  0D3D	    0D2B		8300
	[	  1A5B	    1A7B      1A1B	1A00
	]	  1B5D	    1B7D      1B1D	1B00
	;	  273B	    273A		2700
	'	  2827	    2822
	`	  2960	    297E
	\	  2B5C	    2B7C      2B1C	2600 (same as Alt L)
	,	  332C	    333C
	.	  342E	    343E
	/	  352F	    353F

       键位      常态      上档态    控制态    变更态

	F1	  3B00	    5400      5E00	6800
	F2	  3C00	    5500      5F00	6900
	F3	  3D00	    5600      6000	6A00
	F4	  3E00	    5700      6100	6B00
	F5	  3F00	    5800      6200	6C00
	F6	  4000	    5900      6300	6D00
	F7	  4100	    5A00      6400	6E00
	F8	  4200	    5B00      6500	6F00
	F9	  4300	    5C00      6600	7000
	F10	  4400	    5D00      6700	7100
	F11	  8500	    8700      8900	8B00
	F12	  8600	    8800      8A00	8C00

        键位        常态      上档态    控制态    变更态 

	BackSpace    0E08      0E08	 0E7F	  0E00
	Del	     5300      532E	 9300	  A300
	Down Arrow   5000      5032	 9100	  A000
	End	     4F00      4F31	 7500	  9F00
	Enter	     1C0D      1C0D	 1C0A	  A600
	Esc	     011B      011B	 011B	  0100
	Home	     4700      4737	 7700	  9700
	Ins	     5200      5230	 9200	  A200
	Keypad 5		4C35	 8F00
	Keypad *     372A		 9600	  3700
	Keypad -     4A2D      4A2D	 8E00	  4A00
	Keypad +     4E2B      4E2B		  4E00
	Keypad /     352F      352F	 9500	  A400
	Left Arrow   4B00      4B34	 7300	  9B00
	PgDn	     5100      5133	 7600	  A100
	PgUp	     4900      4939	 8400	  9900
	PrtSc				 7200
	Right Arrow  4D00      4D36	 7400	  9D00
	SpaceBar     3920      3920	 3920	  3920
	Tab	     0F09      0F00	 9400	  A500
	Up Arrow     4800      4838	 8D00	  9800


  一些组合键不是在所有系统中都能获取。PS/2 包括了很多不能在PC, XT和 
  AT上获取的组合键。

-   由扫描码检索出字符可以用 0x00FF 和该字符进行逻辑与操作。

-   参见INT16 通码



2.    转自“http://www.hoppie.nl/ivan/keycodes.txt”:


     Keystroke                  Keypress code
--------------------------------------------------
     Esc                        011B
     1                          0231
     2                          0332
     3                          0433
     4                          0534
     5                          0635
     6                          0736
     7                          0837
     8                          0938
     9                          0A39
     0                          0B30
     -                          0C2D
     =                          0D3D
     Backspace                  0E08
     Tab                        0F09
     q                          1071
     w                          1177
     e                          1265
     r                          1372
     t                          1474
     y                          1579
     u                          1675
     i                          1769
     o                          186F
     p                          1970
     [                          1A5B
     ]                          1B5D
     Enter                      1C0D
     Ctrl                         **
     a                          1E61
     s                          1F73
     d                          2064
     f                          2166
     g                          2267
     h                          2368
     j                          246A
     k                          256B
     l                          266C
     ;                          273B
     '                          2827
     `                          2960
     Shift                        **
     \                          2B5C
     z                          2C7A
     x                          2D78
     c                          2E63
     v                          2F76
     b                          3062
     n                          316E
     m                          326D
     ,                          332C
     .                          342E
     /                          352F
     Gray *                     372A
     Alt                          **
     Space                      3920
     Caps Lock                    **
     F1                         3B00
     F2                         3C00
     F3                         3D00
     F4                         3E00
     F5                         3F00
     F6                         4000
     F7                         4100
     F8                         4200
     F9                         4300
     F10                        4400
     F11                        8500
     F12                        8600
     Num Lock                     **
     Scroll Lock                  **
     White Home                 4700
     White Up Arrow             4800
     White PgUp                 4900
     Gray -                     4A2D
     White Left Arrow           4B00
     Center Key                 4C00
     White Right Arrow          4D00
     Gray +                     4E2B
     White End                  4F00
     White Down Arrow           5000
     White PgDn                 5100
     White Ins                  5200
     White Del                  5300
     SysReq                       **
     Key 45 [1]                 565C
     Enter (number keypad)      1C0D
     Gray /                     352F
     PrtSc                        **
     Pause                        **
     Gray Home                  4700
     Gray Up Arrow              4800
     Gray Page Up               4900
     Gray Left Arrow            4B00
     Gray Right Arrow           4D00
     Gray End                   4F00
     Gray Down Arrow            5000
     Gray Page Down             5100
     Gray Insert                5200
     Gray Delete                5300

     Shift Esc                  011B
     !                          0221
     @                          0340
     #                          0423
     $                          0524
     %                          0625
     ^                          075E
     &                          0826
     * (white)                  092A
     (                          0A28
     )                          0B29
     _                          0C5F
     + (white)                  0D2B
     Shift Backspace            0E08
     Shift Tab (Backtab)        0F00
     Q                          1051
     W                          1157
     E                          1245
     R                          1352
     T                          1454
     Y                          1559
     U                          1655
     I                          1749
     O                          184F
     P                          1950
     {                          1A7B
     }                          1B7D
     Shift Enter                1C0D
     Shift Ctrl                   **
     A                          1E41
     S                          1F53
     D                          2044
     F                          2146
     G                          2247
     H                          2348
     J                          244A
     K                          254B
     L                          264C
     :                          273A
     "                          2822
     ~                          297E
     |                          2B7C
     Z                          2C5A
     X                          2D58
     C                          2E43
     V                          2F56
     B                          3042
     N                          314E
     M                          324D
     <                          333C
     >                          343E
     ?                          353F
     Shift Gray *               372A
     Shift Alt                    **
     Shift Space                3920
     Shift Caps Lock              **
     Shift F1                   5400
     Shift F2                   5500
     Shift F3                   5600
     Shift F4                   5700
     Shift F5                   5800
     Shift F6                   5900
     Shift F7                   5A00
     Shift F8                   5B00
     Shift F9                   5C00
     Shift F10                  5D00
     Shift F11                  8700
     Shift F12                  8800
     Shift Num Lock               **
     Shift Scroll Lock            **
     Shift 7 (number pad)       4737
     Shift 8 (number pad)       4838
     Shift 9 (number pad)       4939
     Shift Gray -               4A2D
     Shift 4 (number pad)       4B34
     Shift 5 (number pad)       4C35
     Shift 6 (number pad)       4D36
     Shift Gray +               4E2B
     Shift 1 (number pad)       4F31
     Shift 2 (number pad)       5032
     Shift 3 (number pad)       5133
     Shift 0 (number pad)       5230
     Shift . (number pad)       532E
     Shift SysReq                 **
     Shift Key 45 [1]           567C
     Shift Enter (number pad)   1C0D
     Shift Gray /               352F
     Shift PrtSc                  **
     Shift Pause                  **
     Shift Gray Home            4700
     Shift Gray Up Arrow        4800
     Shift Gray Page Up         4900
     Shift Gray Left Arrow      4B00
     Shift Gray Right Arrow     4D00
     Shift Gray End             4F00
     Shift Gray Down Arrow      5000
     Shift Gray Page Down       5100
     Shift Gray Insert          5200
     Shift Gray Delete          5300

     Ctrl Esc                   011B
     Ctrl 1                       --
     Ctrl 2 (NUL)               0300
     Ctrl 3                       --
     Ctrl 4                       --
     Ctrl 5                       --
     Ctrl 6 (RS)                071E
     Ctrl 7                       --
     Ctrl 8                       --
     Ctrl 9                       --
     Ctrl 0                       --
     Ctrl -                     0C1F
     Ctrl =                       --
     Ctrl Backspace (DEL)       0E7F
     Ctrl Tab                   9400
     Ctrl q (DC1)               1011
     Ctrl w (ETB)               1117
     Ctrl e (ENQ)               1205
     Ctrl r (DC2)               1312
     Ctrl t (DC4)               1414
     Ctrl y (EM)                1519
     Ctrl u (NAK)               1615
     Ctrl i (HT)                1709
     Ctrl o (SI)                180F
     Ctrl p (DEL)               1910
     Ctrl [ (ESC)               1A1B
     Ctrl ] (GS)                1B1D
     Ctrl Enter (LF)            1C0A
     Ctrl a (SOH)               1E01
     Ctrl s (DC3)               1F13
     Ctrl d (EOT)               2004
     Ctrl f (ACK)               2106
     Ctrl g (BEL)               2207
     Ctrl h (Backspace)         2308
     Ctrl j (LF)                240A
     Ctrl k (VT)                250B
     Ctrl l (FF)                260C
     Ctrl ;                       --
     Ctrl '                       --
     Ctrl `                       --
     Ctrl Shift                   **
     Ctrl \ (FS)                2B1C
     Ctrl z (SUB)               2C1A
     Ctrl x (CAN)               2D18
     Ctrl c (ETX)               2E03
     Ctrl v (SYN)               2F16
     Ctrl b (STX)               3002
     Ctrl n (SO)                310E
     Ctrl m (CR)                320D
     Ctrl ,                       --
     Ctrl .                       --
     Ctrl /                       --
     Ctrl Gray *                9600
     Ctrl Alt                     **
     Ctrl Space                 3920
     Ctrl Caps Lock               --
     Ctrl F1                    5E00
     Ctrl F2                    5F00
     Ctrl F3                    6000
     Ctrl F4                    6100
     Ctrl F5                    6200
     Ctrl F6                    6300
     Ctrl F7                    6400
     Ctrl F8                    6500
     Ctrl F9                    6600
     Ctrl F10                   6700
     Ctrl F11                   8900
     Ctrl F12                   8A00
     Ctrl Num Lock                --
     Ctrl Scroll Lock             --
     Ctrl White Home            7700
     Ctrl White Up Arrow        8D00
     Ctrl White PgUp            8400
     Ctrl Gray -                8E00
     Ctrl White Left Arrow      7300
     Ctrl 5 (number pad)        8F00
     Ctrl White Right Arrow     7400
     Ctrl Gray +                9000
     Ctrl White End             7500
     Ctrl White Down Arrow      9100
     Ctrl White PgDn            7600
     Ctrl White Ins             9200
     Ctrl White Del             9300
     Ctrl SysReq                  **
     Ctrl Key 45 [1]            --  
     Ctrl Enter (number pad)    1C0A
     Ctrl / (number pad)        9500
     Ctrl PrtSc                 7200
     Ctrl Break                 0000
     Ctrl Gray Home             7700
     Ctrl Gray Up Arrow         8DE0
     Ctrl Gray Page Up          8400
     Ctrl Gray Left Arrow       7300
     Ctrl Gray Right Arrow      7400
     Ctrl Gray End              7500
     Ctrl Gray Down Arrow       91E0
     Ctrl Gray Page Down        7600
     Ctrl Gray Insert           92E0
     Ctrl Gray Delete           93E0

     Alt Esc                    0100
     Alt 1                      7800
     Alt 2                      7900
     Alt 3                      7A00
     Alt 4                      7B00
     Alt 5                      7C00
     Alt 6                      7D00
     Alt 7                      7E00
     Alt 8                      7F00
     Alt 9                      8000
     Alt 0                      8100
     Alt -                      8200
     Alt =                      8300
     Alt Backspace              0E00
     Alt Tab                    A500
     Alt q                      1000
     Alt w                      1100
     Alt e                      1200
     Alt r                      1300
     Alt t                      1400
     Alt y                      1500
     Alt u                      1600
     Alt i                      1700
     Alt o                      1800
     Alt p                      1900
     Alt [                      1A00
     Alt ]                      1B00
     Alt Enter                  1C00
     Alt Ctrl                     **
     Alt a                      1E00
     Alt s                      1F00
     Alt d                      2000
     Alt f                      2100
     Alt g                      2200
     Alt h                      2300
     Alt j                      2400
     Alt k                      2500
     Alt l                      2600
     Alt ;                      2700
     Alt '                      2800
     Alt `                      2900
     Alt Shift                    **
     Alt \                      2B00
     Alt z                      2C00
     Alt x                      2D00
     Alt c                      2E00
     Alt v                      2F00
     Alt b                      3000
     Alt n                      3100
     Alt m                      3200
     Alt ,                      3300
     Alt .                      3400
     Alt /                      3500
     Alt Gray *                 3700
     Alt Space                  3920
     Alt Caps Lock                **
     Alt F1                     6800
     Alt F2                     6900
     Alt F3                     6A00
     Alt F4                     6B00
     Alt F5                     6C00
     Alt F6                     6D00
     Alt F7                     6E00
     Alt F8                     6F00
     Alt F9                     7000
     Alt F10                    7100
     Alt F11                    8B00
     Alt F12                    8C00
     Alt Num Lock                 **
     Alt Scroll Lock              **
     Alt Gray -                 4A00
     Alt Gray +                 4E00
     Alt 7 (number pad)           # 
     Alt 8 (number pad)           # 
     Alt 9 (number pad)           # 
     Alt 4 (number pad)           # 
     Alt 5 (number pad)           # 
     Alt 6 (number pad)           # 
     Alt 1 (number pad)           # 
     Alt 2 (number pad)           # 
     Alt 3 (number pad)           # 
     Alt Del                      --
     Alt SysReq                   **
     Alt Key 45 [1]               --
     Alt Enter (number pad)     A600
     Alt / (number pad)         A400
     Alt PrtSc                    **
     Alt Pause                    **
     Alt Gray Home              9700
     Alt Gray Up Arrow          9800
     Alt Gray Page Up           9900
     Alt Gray Left Arrow        9B00
     Alt Gray Right Arrow       9D00
     Alt Gray End               9F00
     Alt Gray Down Arrow        A000
     Alt Gray Page Down         A100
     Alt Gray Insert            A200
     Alt Gray Delete            A300

  -------------------------------------------------------------------------

脚注
        [1]   在美国，101/102键键盘有101 个键。海外版本有一个附加的键，夹在
	      左上档键和Z 键之间。此附加键是由IBM 确定的（在本表中是 45 键）。              

        [**]  键及键组合若有 ** 标记，则被ROM BIOS所使用，但不会将键值放入键盘
	      缓冲区。

        [--]  键及键组合若有 -- 标记，则被ROM BIOS所忽略。



3. 转自“http://heim.ifi.uio.no/~stanisls/helppc/make_codes.html”:

INT 9 - 硬件键盘的通/断码

        键位         通码  断码                 键位   通码  断码

	Backspace     0E    8E			F1	3B    BB
	Caps Lock     3A    BA			F2	3C    BC
	Enter	      1C    9C			F3	3D    BD
	Esc	      01    81			F4	3E    BE
	Left Alt      38    B8			F7	41    C1
	Left Ctrl     1D    9D			F5	3F    BF
	Left Shift    2A    AA			F6	40    C0
	Num Lock      45    C5			F8	42    C2
	Right Shift   36    B6			F9	43    C3
	Scroll Lock   46    C6			F10	44    C4
	Space	      39    B9			F11	57    D7
	Sys Req (AT)  54    D4			F12	58    D8
	Tab	      0F    8F

                    数字小键盘键位             通码   断码

		    Keypad 0  (Ins)		52	D2
		    Keypad 1  (End) 		4F	CF
		    Keypad 2  (Down arrow)	50	D0
		    Keypad 3  (PgDn)		51	D1
		    Keypad 4  (Left arrow)	4B	CB
		    Keypad 5			4C	CC
		    Keypad 6  (Right arrow)	4D	CD
		    Keypad 7  (Home)		47	C7
		    Keypad 8  (Up arrow)	48	C8
		    Keypad 9  (PgUp)		49	C9
		    Keypad .  (Del) 		53	D3
		    Keypad *  (PrtSc)		37	B7
		    Keypad -			4A	CA
		    Keypad +			4E	CE

               键位   通码  断码               键位   通码  断码

		A      1E    9E 		N      31    B1
		B      30    B0 		O      18    98
		C      2E    AE 		P      19    99
		D      20    A0 		Q      10    90
		E      12    92 		R      13    93
		F      21    A1 		S      1F    9F
		G      22    A2 		T      14    94
		H      23    A3 		U      16    96
		I      17    97 		V      2F    AF
		J      24    A4 		W      11    91
		K      25    A5 		X      2D    AD
		L      26    A6 		Y      15    95
		M      32    B2 		Z      2C    AC

               键位   通码  断码               键位   通码  断码

		1      02    82 		-      0C    8C
		2      03    83 		=      0D    8D
		3      04    84 		[      1A    9A
		4      05    85 		]      1B    9B
		5      06    86 		;      27    A7
		6      07    87 		'      28    A8
		7      08    88 		`      29    A9
		8      09    89 		\      2B    AB
		9      0A    8A 		,      33    B3
		0      0B    8B 		.      34    B4
						/      35    B5


键盘扩展键 （101/102 键）

        控制键                    通码            断码

	Alt-PrtSc (SysReq)	  54		  D4
	Ctrl-PrtSc		  E0 37 	  E0 B7
	Enter			  E0 1C 	  E0 9C
	PrtSc			  E0 2A E0 37	  E0 B7 E0 AA
	Right Alt		  E0 38 	  E0 B8
	Right Ctrl		  E0 1D 	  E0 9D
	Shift-PrtSc		  E0 37 	  E0 B7
	/			  E0 35 	  E0 B5
	Pause			  E1 1D 45 E1 9D C5  (not typematic)
	Ctrl-Pause (Ctrl-Break)   E0 46 E0 C6	     (not typematic)

	- 键位是以未使用断码扫描码字节信息的“非机器自动连续打印的"所
          生成的一种扫描码字节流来标记的。（实际上断码是通码的一部分）
          （译注：typematic 有人翻译为‘机打’）


                        常态或上档态及 
                        数字键盘锁定态模式

                                                  数字键盘锁定
					      Make	    Break
        键位		 通码    断码	      通码           断码

	Del		 E0 53	 E0 D3	   E0 2A E0 53	 E0 D3 E0 AA
	Down arrow	 E0 50	 E0 D0	   E0 2A E0 50	 E0 D0 E0 AA
	End		 E0 4F	 E0 CF	   E0 2A E0 4F	 E0 CF E0 AA
	Home		 E0 47	 E0 C7	   E0 2A E0 47	 E0 C7 E0 AA
	Ins		 E0 52	 E0 D2	   E0 2A E0 52	 E0 D2 E0 AA
	Left arrow	 E0 4B	 E0 CB	   E0 2A E0 4B	 E0 CB E0 AA
	PgDn		 E0 51	 E0 D1	   E0 2A E0 51	 E0 D1 E0 AA
	PgUp		 E0 49	 E0 C9	   E0 2A E0 49	 E0 C9 E0 AA
	Right arrow	 E0 4D	 E0 CD	   E0 2A E0 4D	 E0 CD E0 AA
	Up arrow	 E0 48	 E0 C8	   E0 2A E0 48	 E0 C8 E0 AA

        键位             左上档键按下时              右上档键按下时

			 Make	       Break	      Make	    Break
                         通码          断码           通码          断码

	Del	      E0 AA E0 53   E0 D3 E0 2A    E0 B6 E0 53	 E0 D3 E0 36
	Down arrow    E0 AA E0 50   E0 D0 E0 2A    E0 B6 E0 50	 E0 D0 E0 36
	End	      E0 AA E0 4F   E0 CF E0 2A    E0 B6 E0 4F	 E0 CF E0 36
	Home	      E0 AA E0 47   E0 C7 E0 2A    E0 B6 E0 47	 E0 C7 E0 36
	Ins	      E0 AA E0 52   E0 D2 E0 2A    E0 B6 E0 52	 E0 D2 E0 36
	Left arrow    E0 AA E0 4B   E0 CB E0 2A    E0 B6 E0 4B	 E0 CB E0 36
	PgDn	      E0 AA E0 51   E0 D1 E0 2A    E0 B6 E0 51	 E0 D1 E0 36
	PgUp	      E0 AA E0 49   E0 C9 E0 2A    E0 B6 E0 49	 E0 C9 E0 36
	Right arrow   E0 AA E0 4D   E0 CD E0 2A    E0 B6 E0 4D	 E0 CD E0 36
	Up arrow      E0 AA E0 48   E0 C8 E0 2A    E0 B6 E0 48	 E0 C8 E0 36
	/	      E0 AA E0 35   E0 B5 E0 2A    E0 B6 E0 35	 E0 B5 E0 36


	- PS/2 类型有三套通断扫描码。其中第一套是适用于PC & XT 的通断码扫描码集，
          并列在了这里。扫描码集可以通过向8042 键盘控制器（端口60）写入值 F0 来
          选择。下面是扫描码集的简要介绍（更多第2，3套扫描码集的信息见PS/2 技术
          参考手册）

	/  第一套扫描码集，每个键具有一个基本的扫描码。一些键产生扩展扫
           描码以便人工生成上档状态。它与 PC 和 XT 机上的标准扫描码相似。

	/  第二套扫描码集，每个键发送一个通码扫描码和两个断码扫描码字节
          （通码在F0之后）。这套扫描码集在 IBM AT 机上也有效。

	/  第三套扫描码集，每个键发送一个通码扫描码和两个断码扫描码字节
          （通码在F0之后）并且键位不随 Shift/Alt/Ctrl 等键的使用而改变。

	/  typematic scan codes are the same as the make scan code
           “非机器自动连续打印的”扫描码和通码扫描码是相同的。

	- 一些 Tandy 1000 机器在多重组合键的 shift 键被按下时不能处理 ALT 键。
          使用键组合 Alt-Shift-H 时会丢失 ALT 键。

	- 扩展键比如（F11,F12）等只能在具有 BIOS 键盘扩展支持（或 INT 9 扩展）
          的系统上被读取。用INT16读取这些系统上的特别的键时，必须使用10号功能 。


******************************************************************************
***                         GRLDR  错误提示信息                            ***
******************************************************************************
                            

1.  缺少主引导辅助记录。

        紧接在主引导记录后的辅助功能程序不见了，或者是它已经被病毒或 Windows 
        XP/Vista等删除了。

        运行 bootlace.com 工具来解决这个问题。

2.  缺陷太多的BIOS!

        你的 BIOS 太糟糕了。它甚至不能支持 INT 13/AH=8 。

        除了升级你的 BIOS 没有办法解决。未来，缺陷多的 BIOS 将会很常见而且
        会对 grub4dos 造成很多问题。

3.  此分区系统是 NTFS 但包含未知的引导记录。请安装正确的微软 NTFS 引导扇区到
这个分区，或者建立一个FAT12/16/32的分区并将GRLDR 及MENU.LST 的相同的备份
文件放到那里。

	引导记录已经被微软 Windows XP Service Pack 2 改变或删除。

	你可以用原来的引导记录来安装，以清理掉Windows 2K/XP的记录。另一个解决办
        法是，你可以在系统上建立一个FAT分区，并且将GRLDR 和你的MENU.LST复制到它
        的根目录。

	在NTFS分区grldr的自举代码在加载GRLDR时可能会失败，但在FAT分区（甚至在
        ext2/ext3分区）它总能成功加载 GRLDR 。

	注意 NTLDR只能加载grldr的自举代码（即，grldr开头的16个扇区），而不能将
        整个grldr载入。
 

	因此，自从它用于 BOOT.INI 和 NTLDR 以来 ，C 盘根目录下必须存在有GRLDR 
       （这里的C盘可以是 NTFS 文件系统）。



******************************************************************************
***                             已知的BIOS 缺陷                            ***
******************************************************************************
                                

1. 一些较新的 Dell 机不能支持int13/AH=43h 。当你尝试对仿真磁盘进行写访问时，
   可能会遭遇失败。

        注意：这个缺陷非常严重！老的安装方法"root+setup" （在实模式的grub环
        境中）使用INT13将stage2文件写入第一扇区。在这些有缺陷的DELL机上通过
        LBA 模式来访问 stage2 文件时将会失败。

2.一些有缺陷的BIOS不能引导启动光盘映像文件（见前面）。（虚拟机qemu 能良好的引导）

3.在DOS下运行GRUB.EXE时，一些较新的 Dell 机激烈地破坏那些硬件中断请求的中断向量，
   而使得机器会失去响应 。你可以尝试用BADGRUB.EXE来再试一次。

4.有报告称，通过Linux中的kexec启动GRUB.EXE后，一些BIOS将功能异常。报告称一些
   机器会死机，而另一些不能访问USB驱动器。
   
******************************************************************************
***                                 已知问题                               ***
******************************************************************************
                                 
1.	在 Windows 9x/Me的DOS窗口运行GRUB.EXE时可能会死机，特别是在这些系统
	使用USB的时候。在Linux下通过KEXEC运行GRUB.EXE时，你也可能碰到同样的
	问题。

注意：  你不能在已进入保护模式的Win 9x 中运行GRUB.EXE,那可能会死机；作为变
	通，你可以执行“重启并进入MS-DOS模式”选项来达到运行 GRUB.EXE 的目
	的，这很安全。

2.	默认的chainloader动作将保持 A20 的状态。一些有缺陷的 DOS 扩展内存管理软
	件可能会令机器死掉。你可以在chainloader命令行中使用--disable-a20选项
	然后再试一次。至少，你应当避免使用那些有缺陷的内存管理软件。

3.	OEM签名为清华同方，主板为精英L4S5M， BIOS版本1.1a(日期 2002-1-10)
        的机器上，其 int15 含有缺陷，当它启动一个多重引导核心时，会失去响应。
       （比如使用syslinux的memdisk时）

4.	在天汇标准中文系统中，较新的GRUB.EXE不能运行。
	总之，在具有反跟踪措施的内存驻留程序的系统下GRUB.EXE不再运行。

******************************************************************************
***                 二进制文件及对应的源代码文件列表                       ***
******************************************************************************
           
二进制文件      源代码主文件            包含的其他源代码或二进制文件
-------------   ----------------	-------------------------------------

bootlace.com	bootlacestart.S		bootlace.inc, grldrstart.S

grldr		grldrstart.S		pre_stage2(binary, See note below)

grldr.mbr	mbrstart.S		grldrstart.S

grub.exe	dosstart.S		pre_stage2(binary, See note below)

hmload.com	hmloadstart.S

-----------------------------------------------------------------------------
注意：pre_stage2 是GNU GRUB的主体程序，它以二进制格式被简单的添加到grldrstart
      及dosstart部分，形成我们的grldr和grub.exe 。

注意：GRUB （无.exe后缀）在Linux下是一个静态链接的 ELF 格式的可执行文件，它
可以被GRUB Shell正常调用。GRUB Shell 是一个启动管理软件，但并不是一个引导装
载器（boot 命令在GRUB Shell里不能执行）。GRUB.EXE （通过KEXEC）能作为一个引
导装载器直接在 Linux 下使用。

******************************************************************************
***                    GRUB.EXE 返回 DOS 时的内存布局                      ***
******************************************************************************
                
使用 quit 命令实现返回到DOS，是在GRUB.EXE是从DOS启动的情况下。

1.在GRUB.EXE 将控制权移交给 pre_stage2 之前，它将复制 640 kb的常规内存到
  物理地址0x200000 （即，2 M）处，并将立即写入4 字节的长整数到常规内存备
  份区之后：
	At 0x2A0000:	0x50554B42, it is the "BKUP" signature.
                        0x50554b42, 它是“BKUP”的签名 。

	At 0x2A0004:	Gate A20 status under DOS: non-zero means A20 on;
			zero means A20 off. Update: A20 always on, see below.
                        DOS下的A20地址线门状态：非零表示A20开启；零表示A20 
                        地址线关闭。更新：A20 始终开启，参见后面。

	At 0x2A0008:	high word is boot-CS, low word is boot-IP. The quit
			command uses this entry point to return to DOS.
                        高字节是引导的代码段段地址，低字节是引导的指令指针值。
                        退出命令 quit 使用这个入口点返回DOS。

	At 0x2A000C:	CheckSum: the sum of all long integers in the memory
			range from 0x200000 to 0x2A000F is 0.
                        校验和：内存范围从0x200000 到 0x2A000F的所有长整数的
                        和为 0 。

2.如果上述内存结构被某个grub命令所破坏，quit命令将发出一条错误提示信息而拒绝从grub中返回DOS。

3.由于GRUB可能破坏扩展内存，在DOS下你最好避免在GRUB.EXE运行前使用扩展内存。
  
4. Gate A20 will be enabled by GRUB.EXE. Hopefully this would hurt nothing.
   GRUB.EXE将开启A20 地址线。真希望这不会危及任何东西。

******************************************************************************
***                    常规内存/低端内存空间的内存使用                     ***
******************************************************************************
                

1. boot.c, fsys_reiserfs.c: 8K below 0x68000.

2. fsys_ext2fs.c, fsys_minix.c: 1K below 0x68000.

3. fsys_jfs.c: 4K + 256 bytes below 0x68000.

4. fsys_reiserfs.c: 202 bytes at 0x600.

5. fsys_xfs.c: 188 bytes at 0x600.

6. fsys_xfs.c: (logical block size) bytes below 0x68000.

7. geometry tune: 0x50000 - 0x67fff.

******************************************************************************
***                       关于GRUB.EXE的命令行长度                         ***
******************************************************************************
                   
GRUB.EXE 可以通过CONFIG.SYS中的DEVICE命令来启动：

	DEVICE=grub.exe [--config-file="FILENAME_OR_COMMANDS"]

1. 如果GRUB.EXE是被DEVICE命令调用而且FILENAME_OR_COMMANDS 参数是一个由分号分
   隔的grub命令集合,那么FILENAME_OR_COMMANDS可以接近4 KB长 ----很吃惊?但这是
   事实！MS-DOS 7及以上版本即使允许更长的行，但看起来4 KB对GRUB.EXE足够了。
   当我们希望将一个大菜单嵌入到命令行时，这是非常有用的。注意 grldr 还不支持
   任何命令行参数。

2. 如果GRUB.EXE是被INSTALL命令调用，那么选项长度的限制是80个字符（包括开头的
   --config-file= 这部分）。超出的话可能会立即将MS-DOS挂起。

3. 如果GRUB.EXE是被SHELL命令调用，那么选项长度的的限制是126个字符（包括开头的
   --config-file= 这部分）。超出的话虽然不会将 MS-DOS 挂起，但命令行将被截短。
   这和 DOS 控制台或批处理文件中命令的限制是一样的。

4. DOS编辑器EDIT不支持一行4KB的长度。所以请使用其他编辑器，例如，vi for Linux 。

5. DEVICE=GRUB.EXE 这一行可以和其他的DEVICE命令同时使用，如DEVICE=HIMEM.SYS 
   及DEVICE=EMM386.EXE等。配置命令里的GRUB.EXE所在行必须出现在EMM386.EXE所在
   行的前面，以避免因EMM386而冲突。
   更新 ：从0.4.2版本起，GRUB.EXE在EMM386.EXE加载后，仍然可以运行。

6. 在以上提到的任何情况下，你都可以通过quit命令返回到DOS 。

7. 命令行菜单的内存占用：4KB的命令行菜单起始于物理地址0x0800而终止于0x17ff。

******************************************************************************
***                 DEFAULT 及 SAVEDEFAULT 命令的新语法                    ***
******************************************************************************
             
相对于原来的用法"default NUM"及"default saved "增加的部分，现在有一个新用
法"default FILE"，象这样:

		default (hd0,0)/default

注意参数FILE必须是一个有效的DEFAULT文件格式。一个简单的DEFAULT文件就包含
在发行版中。你可以复制它到你希望的地方，但是你应该避免手工修改它的大小。
DEFAULT文件可以按以下方法使用： 
(1) 首先，你要复制一个格式有效的default文件到你运行的系统上。

(2) 其次，你要使用GRUB中的"default FILE"命令来表明是使用这个FILE作为我们新
    的预设文件，以便"savedefault"命令执行时写入它。


(3)  然后，你可以使用"savedefault"命令来把想要的入口数字保存到这个新的预设文
    件中。

(4) 下次启动时，你可以通过使用类似上面第二步骤中的"default FILE"一样的
    命令来读取已保存的入口数字。

同时，SAVEDEFAULT 命令增加了一个选项 --wait=T ,象这样：

		savedefault --wait=5

如果`--wait=T' 选项被指定而且 T 非零，savedefault 命令将在它就要写入磁盘
前，给使用者一个提示信息。

这里是一个简单的menu.lst文件：

#--------------------begin menu.lst---------------------------------------
color black/cyan yellow/cyan
timeout 30
default /default

title find and load NTLDR of Windows NT/2K/XP
find --set-root /ntldr
chainloader /ntldr
savedefault --wait=2

title find and load CMLDR, the Recovery Console of Windows NT/2K/XP
fallback 2
find --set-root /cmldr
chainloader /cmldr
#####################################################################
# write string "cmdcons" to memory 0000:7C03 in 2 steps:
#####################################################################
# step 1. Write 4 chars "cmdc" at 0000:7C03
write 0x7C03 0x63646D63
# step 2. Write 3 chars "ons" and an ending null at 0000:7C07
write 0x7C07 0x00736E6F
savedefault --wait=2

title find and load IO.SYS of Windows 9x/Me
find --set-root /io.sys
chainloader /io.sys
savedefault --wait=2

title floppy (fd0)
chainloader (fd0)+1
rootnoverify (fd0)
savedefault --wait=2

title find and boot Linux with menu.lst already installed
find --set-root /sbin/init
savedefault --wait=2
configfile /boot/grub/menu.lst

title find and boot Mandriva with menu.lst already installed
find --set-root /etc/mandriva-release
savedefault --wait=2
configfile /boot/grub/menu.lst

title back to dos
savedefault --wait=2
quit

title commandline
savedefault --wait=2
commandline

title reboot
savedefault --wait=2
reboot

title halt
savedefault --wait=2
halt
#--------------------end menu.lst---------------------------------------

注意 1：预设文件 DEFAULT 必须是存在的而且具有和前面所述一样严格的格式。

注意 2：在一个有 MENU.LST 文件的相同目录中的 DEFAULT 文件将和 MENU.LST 文
        件一起被联合调用

注意 3：即使没有出现`default'命令，被关联的 DEFAULT 文件也将自动生效。

注意 4：就在菜单文件（诸如，GRLDR 的关联文件MENU.LST,或是通过
        `grub.exe --config-file=(DEVICE)/PATH/YOUR_MENU_FILE'来指定的，
        或是通过grub的`configfile'命令来指定的）取得控制权之前，它的
        关联文件DEFAULT只要出现就会被使用，直到遇见了一个明确的`default'命令。

******************************************************************************
***                   新的 `cdrom' 命令的语法                              ***
******************************************************************************
                      
1. 初始化ATAPI接口的CDROM设备：

	grub> cdrom --init

   显示找到的atapi接口的cdrom光驱的数目：参数为 atapi_dev_count

2. 停止ATAPI接口的CDROM设备：

	grub> cdrom --stop

   这会设置参数atapi_dev_count为0 。

3. 增加搜索atapi cdrom设备的IO端口。例如：

	grub> cdrom --add-io-ports=0x03F601F0

   在执行`cdrom --init'以及`map --hook'命令后，cdrom光驱可以通过(cd0), 
   (cd1), ...等设备号来访问。

注意 1：如果系统不完全支持ATAPI CD-ROM 规范，在你试图访问这些（cdX）设备时将
        遭遇失败。

注意 2：在执行一条`cdrom --stop'命令后，你应当使用一条`map --unhook'命令。当然，
        你可以再次使用`map --hook'命令，假如还有驱动器被映射着的话。

注意 3：在增加IO端口之后，你应当接着`cdrom --init'执行一条`map --unhook'命令然
        后再接着执行一条`map --hook'命令。

        默认将使用这些端口来搜索cdrom设备（因此不需要再添加了）
		0x03F601F0, 0x03760170, 0x02F600F0,
		0x03860180, 0x6F006B00, 0x77007300.

注意 4：BIOS可能已经提供了cdrom 的接口。它的设备号总是（cd）。在 `cdrom --init' 
        和 `map --hook' 执行后，我们可以有我们自己有效的(cd0), (cd1), ...等设备。

注意 5：你可以用块列表的方式去访问(cd)和 (cdX) 等设备。例子:

		cat --hex (cd0)16+2

        cdrom 扇区是大小为 2048 字节的大扇区。

注意 6：我们的iso9660文件系统驱动具有Rock-Ridge扩展支持。

注意 7：现在，（cd）及 （cdX）设备可以被引导了。示例：
		chainloader (cd)
		boot

		chainloader (cd0)
		boot

		chainloader (cd1)
		boot

	在chainloader (cd)之前，你必须保证已经可以访问该设备。

******************************************************************************
***                          关于新命令 `setvbe'                           ***
******************************************************************************
                      
Gerardo Richarte 先生提供了'setvbe'的源码，下面是注释：

	
        'setvbe'是一个新的命令，它可以在系统核心运行前被用来改变视频模式。

        例如，你可以执行

		setvbe 1024x768x32

	这会扫描出其可用模式的列表并设置它，并且自动在随后的每个kernel命令
        行中增加一个选项`video='。增加的选项`video='类似于：

		video=1024x768x32@0xf0000000,4096

	这里的0xf0000000是vbe报告的视频模式的帧缓存地址，而4096是扫描线的字节大小。

	如果你想在你的操作系统上获得一些图形支持，但是除了只写一个像素点到视频内
        存而外，你却不想使用任何视频功能，这确实有用。


******************************************************************************
***                        关于DOS工具'hmload'                             ***
******************************************************************************

此程序由 John Cobb 先生编写（伦敦玛丽皇后学院）。

John Cobb先生的注释：

	为了使用内存驱动器的特性，我写了一个程序“hmload”来将任意文件加载
        到高端内存的任意地址。这个程序不是十分深奥但依赖在XMS 里开启A20地址线。
        （并且必须将那些已经使用了的任何内存空间精心的清理干净）

	我们在linux下生成一个磁盘映象“dskimg”（包含kernel和initrd及一个分区表）。

	我们的引导过程看起来是下面这样：

	hmload -fdskimg -a128
	fixrb
	<unload network drivers>
	grub

		map --ram-drive=0x81
		map --rd-base=0x8000000
		map --rd-size=0x400000
		root (rd,0)
		kernel /kernel root=/dev/ram0 rw ip=bootp ramdisk_size=32768 ...
		initrd /initrd
		boot

详情参阅 http://sysdocs.stu.qmul.ac.uk/sysdocs/Comment/GrubForDOS/

更新   2007-12-05 ：

	现在，MAP命令可以处理gzip压缩（RD）的映像。用的hmload工具，人们可以使用此功能。例如，
        
        步骤 1. 在DOS的相对较低的地址处加载gzip压缩映像：

		hmload -fdskimg.gz -a16

        步骤 2  卸载网络驱动器

        步骤 3.  运行 GRUB.EXE

        步骤 4.  在grub 命令提示符下，执行下列命令：

		map --rd-base=0x1000000	# set rd-base address to be 16M
                                        # 设置rd-base地址为16M

		map --rd-size=<the accurate size of dskimg.gz in bytes>
                              < dsking.gz 精确的字节数 >

		map (rd)+1 (hd0)	# This will decompress (rd) and place
					# the decompressed image at the top end
					# of the extended memory. The (rd)+1
					# here has special meaning and stands
					# for the whole (rd) device. You must
					# use (rd)+1 instead of (rd).
                                        # 这会将（rd）解压并且把解压后的映像放到扩展内存的顶端。
                                        # 这里的(rd)+1具有特定的含义而且将整个（rd）设备放到了顶端。
                                        # 这里你必须使用 (rd)+1 来替代(rd)。
		map --hook
		root (hd0,0)
		kernel /kernel root=/dev/ram0 rw ip=bootp ramdisk_size=32768 ...
		initrd /initrd
		map --unhook
		map (hd0) (hd0)		# Delete the map; this is needed.
                                        # 删除map映射；这是需要的
		boot


******************************************************************************
***                              关于堆栈的注释                            ***
******************************************************************************
                         
保护模式与实模式的堆栈被合并到物理地址 0x2000 处。

所有的功能应当最多使用 2K 的堆空间 （0x1800到0x2000）。因此各个子功能部分
应当使用尽可能小的堆以避免堆栈溢出。

不要使用递归功能，因为他们会消耗太多的堆空间。

原来位于0x68000（向下延伸）的保护模式的堆现在不再使用，并且它可以被用于任何目的。


******************************************************************************
***                  CDROM 驱动器上发现的缺陷                              ***
******************************************************************************
                     
似乎 cdrom 应当连接在IDE控制器的主设备通道上。

如果 cdrom 是从设备，读取cdrom扇区的驱动将失败。希望有人能解决这个问题。

******************************************************************************
***                        BIOS 与 （cd）驱动器                            ***
******************************************************************************                           

当BIOS启动一个非模拟模式的可启动的CD-ROM设备时，它会分配一个BIOS驱动器号给这个
CD设备。如果这个CD-ROM使用grldr或stage2_eltorito作为启动映像文件，那么GRUB可以
通过BIOS分配的驱动器号来访问这个CD-ROM 媒体。

BIOS 会分配一个驱动器号给非模拟模式启动的CDROM 设备，即使这个CDROM 是不能启动的。
虚拟机QEMU就是这样处理的。在引导的时候，GRUB4DOS将搜索那些由BIOS分配的，驱动器号
从0x80至0xFF的，可能存在的非模拟模式的CDROM 驱动器。 所以，如果BIOS为CDROM提供了
扩展int13（功能号41h-4eh）接口，那么这个（cd）设备自动在GRUB4DOS 中有效。


******************************************************************************
***                磁盘仿真方式发生了巨大变化                              ***
******************************************************************************
                 

磁盘仿真方式自从0.4.2正式版之后已经发生了巨大变化。在使用磁盘仿真功能时候，
请不要将较新的版本和旧的版本混合使用。

较新的版本不会自动卸载之前已经在grub4dos环境建立的仿真盘。GRUB.EXE 的一个
很古老的版本，在将控制权移交给grub主程序（即，pre_stage2）前，将会自动释放
先前建立的仿真盘。


******************************************************************************
***            FreeDOS EMM386 版本2.26 (2006-08-27) VCPI服务的问题         ***
******************************************************************************
               

FreeDOS 的 EMM386 版本2.26 中的VCPI服务，“功能号 AX=DE0Ch-选择从
保护模式到虚拟8086模式”，不能正确的执行（总是死机）。选择之一是，
你用微软的 EMM386 来代替它。

即使emm386已经运行，grub.exe也能够启动。但是如果你试图从grub4dos中通过`quit'
命令来返回DOS，VCPI 服务的DE0C 号功能将被调用。如果是微软的 EMM386 ，接下来的
一切都很正常 。而如果是FreeDOS 的 EMM386 ，那么将会死机。

******************************************************************************
***                          map 命令的新增选项                            ***
******************************************************************************
                    

随着0.4.2 最终版的发布，map 命令有了两个新选项。它们是--safe-mbr-hook=SMH
以及--int13-scheme=SCH 。它们都和Win9x环境下（尽可能稳定的）使用磁盘仿真有关。

SMH参数可以是0或1这个两个值之一。作为默认，SMH参数为1 。如果你在Win9x中遇到
磁盘仿真的问题，你可以插入这样一行到`boot'命令之前，

	map --safe-mbr-hook=0

然后再试一次。

SCH在使用时，也可以取0或1之一的值。作为默认，SCH为1 。如果你在Win9x中遇到
磁盘仿真的问题，你可以插入这样一行到`boot'命令之前， 

	map --int13-scheme=0

然后再试一次。

顺便提醒一下。类似于--safe-mbr-hook和--int13-scheme ，MAP命令中有几个其他
选项可被用以设置全局变量。

	map --floppies=M

其中的M 可以是0 , 1 或者2 。MAP 将把一个恰当的M 值设置在地址0040：0010 处。

	map --harddrives=N

其中的N 可以是从0到127之间的值。MAP将把N 值设置在0040：0075处。

	map --memdisk-raw=RAW

其中的RAW默认为1 。如果RAW=0,将通过`int15/ah=87h'访问内存驱动器。

	map --ram-drive=RD

其中RD默认是0x7F的软驱号。如果随机内存驱动器是一个硬盘驱动器镜像（第一扇区
含有分区表），那么你可以将 RD 设置为大于或等于0x80并且小于0xA0之间的值。
如果是一个光盘镜像，那需要设置为大于或等0xA0并且小于0xFF之间的值。

	map --rd-base=ADDR

	map --rd-size=SIZE

其中的 ADDR 指定出内存映像的物理基地址。SIZE指定出内存映像的字节数大小。ADDR 
默认为0 。SIZE的默认值也是0 ，但是值为0 表示4 GB ,而不是零字节长的磁盘。随机
内存驱动器可以在 GRUB 环境中通过使用 （rd） 设备来访问。


******************************************************************************
***                   关于 map 的新选项 --in-situ                          ***
******************************************************************************
                      
--in-situ被使用于硬盘驱动器映像或者是硬盘驱动器分区。通过--in-situ ,我们可以把
一个逻辑分区象征性的作为一个主分区来使用。

--in-situ 的映射是整个驱动器的映射。它只虚拟出分区表和 DBR 上的BPB里的隐藏扇区数。

尽管磁盘仿真在 win9x 中可能会遇到的各种问题，但在win9x中，in-situ的映射却运行得很好。

注意 --in-situ 的映射不会改变真实的分区表。

示例:
	map --in-situ (hd0,4)+1 (hd0)

******************************************************************************
***                      PARTNEW 命令的语法                                ***
******************************************************************************
                         
除了上述章节的仿真方法而外,你也可以替代选择用 PARTNEW 来建立一个新的主分区。
PARTNEW可以为逻辑分区生成一个新的主分区项（在分区表中）。

例如，
	partnew (hd0,3) 0x07 (hd0,4)+1

这里的（hd0,4）+1 代表了整个（hd0,4）分区。这条命令将建立一个分区类型为 0x07 
的新的主分区（hd0,3），并且它的内容（即数据）和逻辑分区（hd0,4）一样。

就像整个逻辑分区时的情况一样，一个连续的分区映像文件也可以用在PARTNEW 命令中：

	partnew (hd0,3) 0x00 (hd0,0)/my_partition.img

这个 0x00 类型表示这个 MY_PARTITION.IMG 映像文件的分区类型由自动检测确定。
上面的命令将建立一个类型恰当的新的主分区（hd0,3），并且使用这个连续的
  (hd0,0)/my_partition.img 文件中的全部内容（数据）作为它的内容（数据）。

PARTNEW 将自动修正 BPB 中的“隐藏扇区数”并且这个修改是永久的。而且PARTNEW
修改分区表也是永久的。

除了建立分区表项外，PARTNEW也可以用来删除（抹掉，擦除）一个分区表项。例如，

	partnew (hd0,3) 0 0 0

这样，主引导记录中最后一个分区表项将被清空。通常，你可以用"partnew PARTITION 0 0 0"的格式来
抹掉其分区表项，但是已经存储在这个分区中的数据不被影响。

******************************************************************************
***             命令控制符号  `&&' , `||' , `;;' , `&;' , `|;'              ***
******************************************************************************

基本语法:
command1 符号 command2 符号 commandN....

说明:
	&& 或 '&;'	前面的命令返回值为`真`时,继续执行后面的命令.
   || 或 '|;'  前面的命令返回值为`假`时,继续执行后面的命令.
	;;				不管前面的命令返回什么,都会继续执行后面的命令.


	有没有';'的主要区别在于变量的扩展,因为变量扩展是执行一行命令前扩展一次.
	带';'的会把一行的命令分隔成多行来执行.这样执行每个命令时都会被扩展一次.
	可以根据实际情况选用适合的符号

	如果你需要在命令中获取该行命令执行过程之中产生的变量就要用带';'的符号.
	只要有碰到带';'那后面的所有命令都会在新行中执行.

例子:
	set a=aa &; echo %a% && echo test
	相当于
	set a=aa
	echo %a% && echo test

注: 
	1.只要返回值非0为真,否则为假.比如
	read 0x60000 && command2
	command2有可能会不被执行.因为内存地址0x60000的值有可能是0.
	一般情况下命令执行失败时总是返回0(假).所以可以用于判断命令执行的结果.
	2.在菜单中使用这些符号会忽略错误检测,这是一个很有用的功能.
	比如:
		find --set-root /file.ext
	在菜单中使用时可能会返回文件未找到的错误并停止执行.必要的话我们可以使用
		find --set-root /file.ext || echo file not found.
	这个命令在菜单中使用会显示find not found,但不停止执行.


例子:
   1.以下命令输出的是1234
		set a=1234
		set a=abcd && echo %a%
   2.以下命令输出的是abcd
		set a=1234
		set a=abcd &; echo %a%
   3.一个比较有用的例子,获取hd0,0的UUID,必须使用带;的符号,因为需要读取实时变量.
		uuid (hd0,0) &; set UUID=%?%
   
   4.最常用到的嵌套
		find --set-root /file1 || find --set-root /file2 || find --set-root /file3
		如果没有找到file1就继续找file2,还是没有找到就找file3,如果还是没有找到将会失败

	5.复杂的嵌套,如果对这些符号没有完全理解,不建议使用.
		set test=1 &; echo supported! && if not "%test%"=="1" echo Unsupported!


2010-11-04更新: 新增三个操作符号  "|",">",">>"

用法:
	command1 | command2

	command1 > file or nul

	command >> file or nul

提示: GRUB4DOS不会改变文件大小,也不能新建文件,所以文件必须已经存在,并且是可写的,有足够的空间否则将被丢弃.
例子:
	cat /test.txt > /abcd.txt

******************************************************************************
***          三个新命令 is64bit, errnum 和 errorcheck                      ***
******************************************************************************
             
is64bit 和 errnum 命令分别用来检索是否是 64 位的系统和错误值。

errcheck off|on

errorcheck（错误检查）命令控制着错误是否被处理。默认错误检查是开启的 ,即在
错误发生时命令脚本将停止执行。而假如错误检查是关闭的，那么脚本将一直执行到 boot 
命令。一条 boot 命令可以把错误检查转变为开启。

******************************************************************************
***                     使用数字键来选择菜单项                            ***
******************************************************************************
                 
例如，如果你想要选择第25项菜单项，你可以先按下数字键2 之后再按下 5 。

******************************************************************************
***                  启动时使用 INSERT 键逐步的调试                        ***
******************************************************************************
              
在一些有缺陷机器上进入 grub4dos 环境时可能会失败。可能是意外的死机或者重启。
在启动时尽可能快的按下 INSERT 键，你就可能获得进入单步启动进程的机会而看到它最
多能运行到哪里，然后请上报这些bug截图 。

******************************************************************************
***                       debug 命令的语法已经改变                         ***
******************************************************************************

DEBUG 命令现在可以用来控制冗余的命令输出：
		debug [ on | off | normal | status | INTEGER ]

0 或者 off 指定为静默模式

1 或者 normal 指定为标准模式

从 2 到 0x7fffffff 或者 on 指定为冗余模式
(调试报告BUG时请使用该模式,可以获得更详细的信息)

******************************************************************************
***                     GRUB4DOS 与 Windows Vista                          ***
******************************************************************************
                        
首先，使用以下命令来建立一个启动项：

	bcdedit /create /d "GRUB for DOS" /application bootsector

执行结果看起来类似这样：
The entry {05d33150-3fde-11dc-a457-00021cf82fb0} was successfully created.

其中长字串{05d33150-3fde-11dc-a457-00021cf82fb0} 是这个项的数字标识{id}。

然后，通过以下命令来设置启动参数：
	bcdedit /set {id} device boot
	bcdedit /set {id} path \grldr.mbr
	bcdedit /displayorder {id} /addlast
请用先前的命令所返回的实际的id 来替换掉 {id}。

最后，复制 GRLDR.MBR 到 你引导分区的根目录下，并且将 GRLDR 和 menu.lst 复制到
任意一个 FAT16/FAT32/EXT2/NTFS 的分区根目录下。

注意：引导分区必须是含有 BOOTMGR 的激活的主分区。

LianJiang 先生写出了一个脚本来自动化的完成这个麻烦的工作：

	@echo off
	rem by lianjiang
	cls
	echo.
	echo   Please run as administrator
	echo.
	pause
	set gname=GRUB for DOS
	set vid=
	set timeout=5
	bcdedit >bcdtemp.txt
	type bcdtemp.txt | find "\grldr.mbr" >nul && echo. && echo  BCD entry existing, no need to install. && pause && goto exit
	bcdedit  /export "Bcd_Backup" >nul
	bcdedit  /create /d "%gname%" /application bootsector >vid.ini
	for,/f,"tokens=2 delims={",%%i,In (vid.ini) Do (
                  set vida=%%i
	)
	for,/f,"tokens=1 delims=}",%%i,In ("%vida%") Do (
                  set vid={%%i}
	)
	echo %vid%>vid.ini
	bcdedit  /set %vid% device boot >nul
	bcdedit  /set %vid% path \grldr.mbr >nul
	bcdedit  /displayorder %vid% /addlast >nul
	bcdedit  /timeout  %timeout% >nul
	if exist grldr.mbr copy grldr.mbr %systemdrive%\ /y && goto exit
	echo.
	echo   Please copy grldr.mbr to %systemdrive%\
	echo.
	pause
	:exit
	del bcdtemp.txt >nul
-------------------------------------------------------------------
更新： Fujianabc 先生指出以下这行
	bcdedit  /set %vid% device boot >nul
必须更改为
	bcdedit  /set %vid% device partition=%SystemDrive% >nul
chenall注: 其实没有必要改,使用boot可以获得更好的兼容性.
-------------------------------------------------------------------
你还需要自行复制 grldr和menu.lst文件。

注意:  你只需要指定BCD的位置就可以修改另一个操作系统的BCD 启动项：
	bcdedit /store D:\boot\BCD ...

注意： 执行这些命令需要提高权限，它们必须是“以管理员身份运行”于cmd.exe中。

注意：已有人报告说，即使使用管理员身份，Vista的某些版本也不支持在C盘根目录下建立
无扩展名的文件。你既可以复制grldr到另外的一个分区来解决这个问题，也可以将 grldr 
重命名，比如为 grub.bin 。如何改名,请参见下节。

******************************************************************************
***                      怎样重命名 grldr                                  ***
******************************************************************************
                         
  grldr 和 grldr.mbr引用引导文件内部的文件名来决定装载哪个文件，所以假如你
  想更换它们的名字，那么你也必须要修改那些内嵌在文件内部的设置。你可以使用
  辅助程序grubinst 来做到这些，grubinst 可以在以下网址下载到：

http://download.gna.org/grubutil/

  grubinst 能生成自定义的grldr.mbr：

	grubinst -o -b=mygrldr C:\mygrldr.mbr

  grubinst 也能编辑一个既有的 grldr 或 grldr.mbr:

	grubinst -e -b=mygrldr C:\mygrldr

	grubinst -e -b=mygrldr C:\mygrldr.mbr

在这种情况中，你必须使用一个和 grub4dos 版本兼容的grubinst，否则修改将会失败。

  所以，在命令中通过加载mygrldr来代替grldr ，你可以使用下面的方法之一：

1.使用已定制好的grldr.mbr 来加载 mygrldr 。在这种情况下，你需要修改内嵌在
  grldr.mbr中的引导文件名。grldr.mbr的名字可以被任意的改变。

2.直接使用mygrldr 。在这种情况下，你需要将 mygrldr 中内嵌的引导文件名改为
  一个合适的名字。

注意： 引导文件名必须遵循 8.3 文件名规范。

******************************************************************************
***                       GRLDR 作为 PXE 启动文件                          ***
******************************************************************************
                          
GRLDR 可以被用作远程或网络服务器的 PXE 启动文件。(pd) 设备被用于访问服务器上文件。
当 GRLDR 已经通过网络启动后，它将使用预设菜单作为配置文件。不过，你可以使用
一条"pxe detect"命令，它的表现是和pxelinux一样的方式。

    * 首先，它将使用设备类型（使用它的 ARP 类型码）和地址来搜索配置文件，全部用
      破折号分割的十六进制；例如，对一个以太网（ARP 类型是1）的88:99:AA:BB:CC:DD 
      地址，它会用文件名01-88-99-AA-BB-CC-DD 来搜索。

    * 其次，它将使用它本地的IP 地址大写字母的十六进制格式（即192.0.2.91 转换为
      C000025B。）来搜索配置文件。如果文件没有找到，它将去掉一个十六进制数字后再试一次。
      最后，它会尝试寻找一个名为 default （小写字母）的文件。

******************************************************************************
***                          PXE 设备                                      ***
******************************************************************************
                             
如果使用PXE启动,GRUB4DOS 将建立一个虚拟设备 (pd)，可能通过它来访问tftp服务器
上的文件。你可以使用下面的步骤来设置一个无盘启动环境：

客户端
你需要从 PXE ROM 上启动。

服务器端
你需要配置一个dhcp服务器和一个tftp服务器。在dhcp服务器上，使用grldr作为引导文件。

你可能希望为不同的客户端加载一个不同的menu.lst 。GRUB4DOS将在以下位置查找配置文件：

	[/mybootdir]/menu.lst/01-88-99-AA-BB-CC-DD
	[/mybootdir]/menu.lst/C000025B
	[/mybootdir]/menu.lst/C000025
	[/mybootdir]/menu.lst/C00002
	[/mybootdir]/menu.lst/C0000
	[/mybootdir]/menu.lst/C000
	[/mybootdir]/menu.lst/C00
	[/mybootdir]/menu.lst/C0
	[/mybootdir]/menu.lst/C
	[/mybootdir]/menu.lst/default

更新1:	如果/mybootdir/menu.lst 文件存在,将会优先使用,这样可以加快引导速度.

这里，我们假设客户端的网卡mac地址是 88:99:AA:BB:CC:DD ，而ip地址是192.0.2.91 (C000025B)。
/mybootdir 是引导文件所在目录，例如，如果引导文件是 /tftp/grldr ，那么mybootdir=tftp 。

如果上面的文件都未出现，grldr将使用它的内置的menu.lst 。

这是一个如何访问tftp服务器上文件的menu.lst文件。

	title Create ramdisk using map
	map --mem (pd)/floppy.img (fd0)
	map --hook
	rootnoverify (fd0)
	chainloader (fd0)+1

	title Create ramdisk using memdisk
	kernel (pd)/memdisk
	initrd (pd)/floppy.img

chenall注: 1.你也可以省略(pd)/或者使用(bd)/或()/
	     这样可以使得一个菜单可以不经过修改就可以用于其它地方的启动.

你可以看到这个 menu.lst 和在普通磁盘上引导的是相似的，你只是需要把象(hd0,0) 
这样的设备用(pd) 来代替。

磁盘设备和 pxe 设备有一些不同点：

1. 你不能把pxe设备上的文件以列表显示。
更新2: 现在可以列表,但要求服务器上有dir.txt文件,使用以下命令可以创建一个dir.txt文件
	dir /b>dir.txt
	也可以直接使用TFTPD32的服务器,选择自动生成DIR.TXT文件.

2.blocklist 命令不能用于 pxe 设备上的文件。

3.如果你想映射一个pxe服务器上的文件，你必须使用--mem 选项 。

当你使用 chainloader 命令装载一个pxe 设备上的文件时，有一个选项你可以使用：

	chainloader --raw (pd)/BOOT_FILE

选项 --raw 的执行就和--force一样，但是它是一次性将文件装载执行。这可以改善
一些情况下的执行效率。

你可以使用 pxe 命令来控制 pxe 设备。

1. pxe
        如果没有使用任何参数，pxe 命令将显示当前设置。

2. pxe blksize N
        设置tftp packet size (传输包)大小。最小值是 512 ，最大值是 1432 。这个参数主要使
        用在那些不支持远大于 512 字节包大小的tftp 服务器上。

3. pxe basedir /dir
	为tftp 服务器上的文件设置基本目录。如
		pxe basedir /tftp

	那么在pxe 设备上的所有文件都和目录 /tftp 相关。例如，(pd)/aa.img 
	对应于服务器上的 /tftp/aa.img 。

	基本目录的默认值是引导文件所在目录，例如，如果引导文件是 /tftp/grldr ，
	那么默认的基本目录就是 /tftp 。

4. pxe keep
	保持 pxe stack。GRUB4DOS的默认退出时自动卸载pxe strack。
	如果你希望在引导后继续使用PXE功能,比如用于RIS安装,这时必须使用这个选项.

5. pxe unload
	立即卸载 PXE stack。pxe占用了大量的常规内存，某些引导程序可能会无法正常引导。
	这时你可以先卸载然后再引导。一个例子：
		title Linux memtest
		map --mem /memtest.bin (rd)
		pxe unload
		kernel (rd)+1
	如果在PXE启动时直接kernel /memtest.bin可能会失败。


******************************************************************************
***                          相对路径支持的新特性                          ***
******************************************************************************
                     
使用`root' 或 `rootnoverify'命令来指定`工作目录' 。

例如：
	root  (hd0,0)/boot/grub

这就指定了当前工作目录是(hd0,0)/boot/grub 。因此所有继"/..."之后的文件名将实际
提交到(hd0,0)/boot/grub/...

也就是说：

	cat  /menu.lst
将等同于
	cat  (hd0,0)/boot/grub/menu.lst


******************************************************************************
***                        N当前根设备的符号                               ***
******************************************************************************
                    
符号`()'可以在访问当前根设备时使用。你可以使用`find --set-root ...'来设置当前根
设备，但find 命令不能设置根设备的`工作目录'。这时你应该使用`()'在find命令后来设
置工作目录。
	root  ()/boot/grub

2008-05-01 更新:
        现在 FIND 命令也可以设置`工作目录'了。例如：

		find  --set-root=/tmp  /boot/grub/menu.lst

        它等同于这一组命令：
		find  --set-root  /boot/grub/menu.lst
		root  ()/tmp

******************************************************************************
***                   map 新选项 --a20-keep-on                             ***
******************************************************************************                     

随着0.4.3最终版的发布，map 有了一个新选项 --a20-keep-on ，它跟内存驱动器扇区访
问后的A20 地址线控制有关

	map --a20-keep-on=0

它必须被使用于"map --hook"命令之前。

作为默认，在INT13 对随机内存的扇区访问之后 A20 将一直开启。如果"map --a20-keep-on=0" 
被使用，那么在INT13 中断调用后的 A20 的状态将和在INT13中断调用前相同。

******************************************************************************
***                   光盘仿真（虚拟化）                                   ***
******************************************************************************
                      
光盘仿真有时候又称为 ISO 仿真。这里是个示例：
	map  (hd0,0)/myiso.iso  (hd32)
	map  --hook
	chainloader  (hd32)
	boot

如果myiso.iso 是不连续的并且你有足够的内存，那么要增加一个--mem选项：
	map  --mem  (hd0,0)/myiso.iso  (hd32)
	map  --hook
	chainloader  (hd32)
	boot

注意：(hd32) 是一个 grub 驱动器，驱动器号和 (0xA0) 等价。如果一个虚拟驱动器被指
定为一个大于或等于0xA0 的驱动器号，那么它将被视为是一个光盘。（即，是 2048 字节
的大扇区）

就像标准的磁盘仿真一样，光盘仿真也（主要）工作于实模式操作系统中。在一个保护模式的
操作系统核心（例如WinNT/2K/XP/VISTA/LINUX）获得控制后，操作系统一般没有能力通过BIOS
的int13 来访问虚拟光盘。

DOS/Win9x 的使用者可以用google搜索到 ELTORITO.SYS 然后将它作为虚拟光驱的设
备驱动使用到CONFIG.SYS 中。

CONFIG.SYS 中 eltorito.sys 的用法举例：
	device=eltorito.sys /D:oemcd001

对应的可能是放在 AUTOEXEC.BAT中的 MSCDEX 命令：
	MSCDEX /D:oemcd001 /L:D

由于在 eltorito.sys中发现了一些缺陷，驱动器可能会加载失败。假如你碰到这类问题，
那么你可以将虚拟光盘的驱动器号从(hd32)更换为(0xFF)然后再试一次。

******************************************************************************
***                          新命令 CHECKRANGE                             ***
******************************************************************************
                     
Checkrang 命令检查一条命令的返回值是否是在指定的值域或排列中。

Usage:		checkrange  RANGE  COMMAND
用法:           checkrange   域    命令

这里是参数 RANGE 的一些示例：
        3 是仅包含数字 3 的值

        3:3 等价于 3 

        3:8 是一个包含数字3, 4, 5, 6, 7, 8的值域
        3,4,5,6,7,8 等同于3:8
        3:5,6:8 也等同于3:8
        3,4:7,8 也等同于3:8

注意：你不能把空格放在值域中。比如：以下是错误的。
	checkrange 1 2 COMMAND

这里用一个示例来演示怎样使用 checkrange 命令：
	checkrange 0x05,0x0F,0x85 parttype (hd0,1) || hide (hd0,1)
这意谓着：如果 (hd0,1) 不是一个扩展分区，那么执行hide (hd0,1)命令隐藏它。

******************************************************************************
***                       新命令 TPM                                       ***
******************************************************************************
                          
"tpm --init"在地址0000:7c00处使用512字节数据作为初始化TPM（可信赖平台模块）的缓存。

在你引导 VISTA 的 BOOTMGR 前，你可能需要在一些机器上使用"tpm --init"。通常你应该在
一条 CHAINLOADR 命令后执行"tpm --init"指令。

******************************************************************************
***               标题间的限制或注释                                       ***
******************************************************************************
                  
把标题用来做限制或注释是可能的。如果一个标题（或菜单项）下所有的菜单命令都是非启动敏感的，
它被叫做是不可启动的。

下面的命令是启动敏感的（而其他命令是非启动敏感的）
	boot
	bootp
	chainloader
	configfile
	embed
	commandline
	halt
	install
	kernel
	pxe
	quit
	reboot
	setup

一个不可启动的标题在使用者按向上方向键或向下方向键时将被跳过。
不可启动的菜单项可以通过使用左方向键或右方向键来被访问（和执行）的。示例：

	title This is an UNBOOTABLE entry(so this line is also a comment)
		pause --wait=0 This title is a comment. Nothing to do.
		pause --wait=0 You can use non-boot-sensitive commands here
		pause --wait=0 of any kind and as many as you would like.
		help
		help root
		help chainloader
		help parttype
		clear
	title ------------------------------------------------------------
		pause --wait=0 This title is a delimitor. Nothing to do.
		pause --wait=0 You can use non-boot-sensitive commands here
		pause --wait=0 of any kind and as many as you would like.
		clear
		help
		help boot
	title ============================================================
		pause --wait=0 This title is a delimitor. Nothing to do.
		pause --wait=0 You can use non-boot-sensitive commands here
		pause --wait=0 of any kind and as many as you would like.
		help
		clear
		help pause
	title ************************************************************
		pause --wait=0 This title is a delimitor. Nothing to do.
		pause --wait=0 You can use non-boot-sensitive commands here
		pause --wait=0 of any kind and as many as you would like.
		help kernel
		help
		clear

注意：一个不可启动菜单项必须至少包含一条命令。如果标题下没有命令，标题将被简单的
丢弃并且不被显示。

******************************************************************************
***                           分支式驱动器                                 ***
******************************************************************************
                              
一些机器在 CHS 和 LBA 模式之间对驱动器实施不同的动作。
当你使用标准的BIOS调用int13/AH=02h来读取扇区时，你可能会发现这个驱动器是一个软盘
但是当你用扩展的BIOS调用(EBIOS)int13/AH=42h来读取扇区时，你会发现是一个光盘。
这样的驱动器被叫做分支式的。

一个分支式的驱动器拥有两个驱动器号：一个是标准的 BIOS 驱动器号十六进制
的 00或FF ，并且这个驱动器只使用 CHS 模式的磁盘访问（标准的BIOS int13/AH=02h）；
另一个是标准的 BIOS 驱动器号（按位与）0x100 （即十进制的256），并且这个驱动器只
使用 LBA 模式的磁盘访问（EBIOS int13/AH=42h）。
例如，驱动器0x00（即，第一软驱）是分支式的.
      那么驱动器（0x00）使用 CHS 模式来访问它的扇区
      而驱动器（0x100）则使用LBA 模式来访问它的扇区。

geometry 命令会用 BIF 代替常见的 CHS 和 LBA 来报告分支式驱动器的磁盘访问模式。

已知的分支式驱动器。发现虚拟机Virtual PC和一些真实机器当它们引导一个软盘模拟模式
的可启动光盘时会建立一个分支式的软驱。命令"geometry (fd0)"将显示：
	drive 0x00(BIF): C/H/S=...Sector Count/Size=.../512

而"geometry (0x100)"将显示
	drive 0x100(BIF): C/H/S=...Sector Count/Size=.../2048

实际上(0x100) 可以访问整个光盘。
你可以执行"ls (0x100)/" 显示光盘上文件（不是那个被引导的软盘映像中的文件）。
当然 "ls (fd0)/"可以列举那些在被引导的软盘映像中的文件。

注意：仅仅是某些（真实的或虚拟的）机器有这样的行为，其他的机器不会产生分支式驱动器。

******************************************************************************
***                       新程序 badgrub.exe                               ***
******************************************************************************
                          
新程序 badgrub.exe 是特意供那些不能运行标准 grub.exe 的‘糟糕的’机器（一些典型
的 DELL 原型机）使用的。


2010-11-04更新: 新增三个操作符号  "|",">",">>"

用法:
	command1 | command2

	command1 > file or nul

	command >> file or nul

提示: GRUB4DOS不会改变文件大小,也不能新建文件,所以文件必须已经存在,并且是可写的,有足够的空间否则将被丢弃.
例子:
	cat /test.txt > /abcd.txt

******************************************************************************
***                           条件查找                                     ***
******************************************************************************
                              
新的find 命令的语法允许带条件的查找设备。

	find [OPTIONS] [FILENAME] [CONDITION]
              选项      文件名     条件

OPTIONS:
	--set-root		set the current root device.
	--set-root=DIR		set current root device and working directory to DIR.
			please also see "Notation For The Current Root Device".
	--ignore-cd		skip search on (cd).
	--ignore-floppies	bypass all floppies.
	--devices=DEVLIST	specify the search devices and order.
		  DEVLIST	u->(ud)
				n->(nd)
				p->(pd)
				h->(hdx)
				c->(cd)
				f->(fdx)
				default: upnhcf


其中的 CONDITION 是一个返回值是 TRUE 或者 FALSE 的标准 grub 命令。

	示例 1: 列举所有的分区，所有的软驱和 (cd) 。

		find

	示例 2：列举文件系统已知的所有设备。

		find +1

	示例 3: 列举分区类型为0xAF的所有分区。

		find checkrange 0xAF parttype

	示例 4：列举分区类型为 0x07 且根目录存在 ntldr 的所有分区。

		find /ntldr checkrange 0x07 parttype

	示例 5: 设置当前根设备到第一个根目录有存在ntldr的分区。

		find --set-root /ntldr

	示例 6: 同例5，但是以下命令只查在硬盘上查找bootmgr

		find --set-root --devices=h /bootmgr

	示例 7: 设置当前根设备为第一激活的主分区。

		find --set-root --devices=h makeactive --status

更新：	新的find 命令语法允许指定要查找和设备和查找的顺序。
	新的参数	--devices=DEVLIST，用于指定查找的设备和顺序。
	DEVLIST可以下以下的字母组合。
	u,p,n,h,c,f -->分别对应 ud,pd,nd,hd,cd,fd,
	查找时根据DEVLIST指定的设备顺序进行查找。默认是upnhcf.
	
	
	例子:	1.只查找硬盘上的文件
		find --devices=h /file
		2.依次查找硬盘、光盘、软盘上的文件
		find --devices=hcf /file

	注意：新的find命令有一个改变，查找的时候会优先查找当前设备（如果在列表中的话）。

******************************************************************************
***                    如何创建 grldr 引导的映像文件                      ***
******************************************************************************
                       

1. 创建1.44M 软盘镜像文件 ext2grldr.img

	dd if=/dev/zero of=ext2grldr.img bs=512 count=2880
	mke2fs ext2grldr.img
	mkdir ext2tmp
	mount -o loop ext2grldr.img ext2tmp
	cp default ext2tmp
	cp menu.lst ext2tmp
	cp grldr ext2tmp
	umount ext2tmp
	bootlace.com --floppy --chs --sectors-per-track=18 --heads=2 --start-sector=0 --total-sectors=2880 ext2grldr.img

2. 创建1.44M 软盘镜像文件 fat12grldr.img

	dd if=/dev/zero of=fat12grldr.img bs=512 count=2880
	mkdosfs fat12grldr.img
	mkdir fat12tmp
	mount -o loop fat12grldr.img fat12tmp
	cp default fat12tmp
	cp menu.lst fat12tmp
	cp grldr fat12tmp
	umount fat12tmp
	bootlace.com --floppy --chs fat12grldr.img

3. 创建 iso9660 文件系统的光盘镜像文件 grldr.iso

	mkdir iso_root
	cp grldr iso_root
	cp menu.lst iso_root
	mkisofs -R -b grldr -no-emul-boot -boot-load-size 4 -o grldr.iso iso_root

   补充1：把 grldr 改名为 grldr.bin ,使用 UltraISO 加载引导文件。

   补充2：使用 UltraISO 加载引导文件 grlgr_cd.bin，然后复制 grldr 文件到根目录。

******************************************************************************
***               使用 bootlace.com 来安装分区引导记录                     ***
******************************************************************************

方法1：

步骤 1. 获取分区的引导扇区然后保存为一个文件 MYPART.TMP 。
	对于EXT2/3/4分区，需要获取起始的3个扇区，对于其他类型的文件系统，你只
	需要获取一个扇区。

步骤 2. 在 DOS、Windows 执行这些命令：
	bootlace.com --floppy MYPART.TMP

步骤 3. 将 MYPART.TMP 写回你原来分区(hdx,y)的引导扇区。


方法2：
	在 DOS 下执行这些命令：
	bootlace.com --install-partition=I K

	I是分区号（0，1，2，3，4，...），K是驱动器号（0x80,0x81，...）。
	执行时会显示简单的磁盘信息和分区容量，提示按“y”键继续，按其他键退出。

在 Linux 下安装引导代码到 PBR：
	bootlace.com --floppy /dev/sda1


注意： 现在只有文件系统（FAT12/16/32/NTFS/ext2/ext3/ext4/exfat）被支持。


注意：grubinst 具有把 grldr 的自举代码安装到分区引导扇区的功能。

******************************************************************************
***                使用一个单一的键来选择菜单项                            ***
******************************************************************************
                    
一些机器具有简化的键盘。这些键盘可能只有数字键 0 到 9 ，外加少数几个其他键。当
菜单还未显示时，使用者可以按下某个键 8 次。当菜单控制模块发现一个连续的单一按
键时，它将认为使用者希望使用这个键来选择菜单和启动。这个单一的键可以充当右方向
键来为使用者选择菜单。然后在使用者停止按键的 5 秒之后，被选择的菜单项将自动启
动。任何的标准键可以被作为单一的键来达到这个目的，除了少数功能键，比如 b ，e ，
回车键，等等。一旦另外的键被按下，单键选择特性将立即消失。


******************************************************************************
***             Parameter file for bootlace running under DOS              ***
******************************************************************************

你可以把所有或部分的命令行参数放到一个文件中。这个文件可以有多行。就像空格
和制表符一样，回车符和换行符也可以在参数文件中分割命令行参数。

示例：
		bootlace < my_parafile
		bootlace --read-only my_mbr < my_other_options

注意： 不能使用管道符"|"。你必须使用输入重定向符 (<) 。

******************************************************************************
***                  使用 bootlace 来建立一个三重的 MBR                   ***
******************************************************************************                     
虽然这也能用于硬盘，但是它典型的使用是被用于 USB 设备。

创建三重的 MBR 的步骤：

1. 使用一个新版的 FDISK 分区软件来建立一个从第 95 扇区开始的FAT12或16或32 的分区
（这里是 LBA 扇区表示法，起始扇区（MBR）是 0 扇区。）

2. 安装 grldr 的引导扇区到这个分区的引导扇区。参见上面的“使用 bootlace.com 来安装分区引导记录”

方法1：对于映像文件

3. 获取从起始扇区 0 扇区（MBR）开始的96个扇区，然后保存到一个文件 MYMBR96.TMP 中。

4. 在DOS或Windows下执行：
	bootlace.com MYMBR96.TMP
5. 将 MYMBR96.TMP 从MBR （0 扇区）开始回写到驱动器上。

方法2：对于磁盘

3. 在DOS下执行：
	bootlace.com 0x80 (或0x81,...)

******************************************************************************
***                    在预置菜单中使用 'pxe detect' 命令                  ***
******************************************************************************
                       
现在“pxe”命令有了个新的子命令“detect”：
		pxe detect [BLOCK_SIZE] [MENU_FILE]
                            包大小选项   菜单文件选项

BLOCK_SIZE 选项指定出 pxe 包的大小。如果它没有被指定或者是被指定为 0 ，那么
grub4dos将通过一个侦测过程来获取数据传送包的一个恰当的的值。

MENU_FILE 选项指定出 PXE 服务器上的配置文件。如果它被省略，在 menu.lst 子目录
中的标准配置文件将获得控制。关于menu.lst 子目录中的配置文件的描述，请查阅上面
的“GRLDR 作为 PXE 启动文件”一节。

如果MENU_FILE 是以"/"开始的，那么PXE 服务器上的 MENU_FILE 将获得控制，否则
（如果MENU_FILE不是以"/"开始）将没有菜单被执行。

在你的系统用 512 字节的默认包大小不能运行时，通常你应该在访问(pd)设备之前
使用一条 "pxe blksize ..." 或 一条 "pxe detect ..."命令。

******************************************************************************
***                    在预置菜单中使用 'configfile'命令                   ***
******************************************************************************
                       
现在预置菜单具有最高控制权。它将在启动设备上的 menu.lst 之前获得控制。如果
'configfile' 命令在初始化命令组中出现，那么控制将转到启动设备上的menu.lst文件。

******************************************************************************
***                    复制文件的新命令 'dd'                               ***
******************************************************************************
                       
用法：
	
dd if=IF of=OF [bs=BS] [count=C] [skip=IN] [seek=OUT] [buf=ADDR] [buflen=SIZE]

将 IF（源文件）复制到OF （目标文件）中。BS 是以字节计数的一个块的大小，默认
值是512 。C 是复制的块数，默认值是源文件中的总块数。IN 指定在读取时跳过的块
数，默认值是 0 。OUT 指定在写入时跳过的块数，默认值是0 。已跳过的块不会被改
变。源文件和目标文件必须是存在的。
译注：新增参数buf表示dd所用的读写缓存的起始地址，buflen表示缓存的长度，即大小。

源文件和目标文件必须以设备名开头，即，`(...)'的格式。对于当前根设备你应该使用`()'。

dd 命令既不扩大也不减小目标文件的大小，源文件尾部剩余的部分将被丢弃。目标
文件不能是gzip压缩过的文件。如果源文件是gzip 压缩过的文件，它将在复制时被
自动解压。

dd 具有危险性，使用风险由你自己的承担。作为一种安全方面的考虑，你应当只使
用 dd 来写入一个内存中的文件。

某些情况下在写入NTFS 中的文件的时，dd 可能会失败。

假如你尝试在菜单中执行dd命令来写入一个不是内存中的设备或者块文件时，你会被安全的拒绝:-)
（更新：现在不再限制）

更新： 新选项实现了让使用者自定义dd 命令的读写缓存。默认读写缓存起始于地
址0x50000，长度为0x10000 （即64KB）。你不能指定起始地址ADDR 低于0x100000
（即 1 MB）的缓存位置。此外，你必须指定参数SIZE 大于0x10000（即64K）。
通常，你需要令ADDR大于或等于0x1000000 （16MB），并且 SIZE 也要大于或等于16MB 。
增大 SIZE 的值能够加快 dd 的读写速度。

更新 2：从 2011 年 10 月 3 日起，缓冲区地址默认位于 1M 处。由于从 2011 年 10 月
20 日开始 grub4dos 保留了 32M 内存作为内部使用，因此通常你想设置 ADDR 使其不低
于 32M。显然，你不应该设置 ADDR 和 SIZE 使其与 grub4dos 的代码和数据发生冲突。

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!!!!
!!!!    Caution! Both IF and OF can be a device name which stands for     !!!!
!!!!    all the sectors on the device. Take utmost care!                  !!!!
!!!!______________________________________________________________________!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        警告！IF 和 OF 都可以是一个设备名，即它代表了设备上全部的扇区。慎之又慎！

******************************************************************************
***                       确认分区的新命令 'uuid'                          ***
******************************************************************************
                 
用法：

	uuid [DEVICE] [UUID]

如果 DEVICE 选项未被指定，将在所有分区中搜索指定的 UUID 号的文件系统，
然后把包含这个文件系统的分区设置为新的根 （如果 uuid 被指定时），或者只列举所
有设备上的文件系统的 uuid 号（如果 uuid 未被指定时）。
如果 DEVICE 选项被指定了，将返回 真 或 假 ，对应于指定的设备是否与指定的 UUID 
号相符（如果uuid被指定时），或者仅仅列举指定设备的uuid 号（uuid 未被指定时）。

示例 1：
	find --set-root uuid () 7f95820f-5e33-4e6c-8f50-0760bf06d79c

这将查找 uuid 等于 7f95820f-5e33-4e6c-8f50-0760bf06d79c的分区，然后将这个找到的分区设置为根。

示例 2：
	uuid ()
这将显示当前根设备的 uuid 号。

******************************************************************************
***                     grub4dos 的 gfxmenu 支持                           ***
******************************************************************************
                        
gfxmenu 支持已经被增加到 grub4dos 当中。使用它，你首先需要找到一个你需要的mesage
文件，然后在menu.lst中用类似这样的命令来装载它：

	gfxmenu /message

这是一个全局命令，也就是说，不能放入任何的菜单项中。同时，它只能被使用于配置文件
中，而在控制台模式中执行它是无效的。

gfxmenu 不能与全局密码保护功能同时使用。

message 文件有两个主要的格式。老的格式是通过gfxboot 3.2版或更旧的版本创建的
（message文件的大小通常只能是150 k），当采用 gfxboot 3.3 版或更新的版本创建
新格式时，（message文件的大小通常可以超过 300K）。这两种格式在grub4dos 中都
已被支持。

******************************************************************************
***           使用 'write' 命令将字符串写入设备或文件中                    ***
******************************************************************************
               
用法：

	write [--offset=SKIP] [--bytes=N] ADDR_OR_FILE INTEGER_OR_STRING

SKIP 是一个整数默认值是 0 。

如果 ADDR_OR_FILE选项 被指定为一个整数，那么它被作为一个内存地址对待，并且
INTEGER_OR_STRING选项也必须是一个整数值。整数 INTEGER_OR_STRING 将被写
入（ADDR_OR_FILE 加上 SKIP 值）的地址处。

默认情况下写入的是一个32位的数值，如果指定了 N 参数，只写入 N*8 位。
例子:
    write --bytes=1 0x8308 0x10   ** 只会改写0x8308处一个字节的值.
    write 0x8308 0x10             ** 会改写32位 0x8308 - 0x830b 4个字节的值.
    write --bytes=8 0x8308 0x10   ** 会改写64位 0x8308 - 0x830F 8个字节的值.


如果 ADDR_OR_FILE选项 指定的是一个设备或一个文件，那么INTEGER_OR_STRING 选
项将被作为一个字符串对待，它将被写入跳过 SKIP 个字节（字节计数）的指定的设
备或文件当中。

14-08-12更新: 现在可以通过--bytes参数限制写入字节数.
例子:
    write --bytes=8 (md)0x300+1 12345678abcdef  ** 只写入12345678

字符串不需要被引用，也就是说，不需要单引号(') 也不用 双引号(") 来引用它。

空格符必须被反斜杠(\)引用。（更新：现在不需要了）
（译注：如果字符串以空格开头，开头的这个空格符还是需要反斜杠引用）

单引号(')和双引号(")不用特别说明并且可以直接使用到字符串中。

下面是一些 C 语言风格的引用序列说明：

	\NNN	（1到3位）八进制值 NNN 表示的字符
	\\	反斜杠
	\a	警报 （声音）
	\b	退格符
	\f	换页符 
	\n	换行符
	\r	回车符
	\t	水平制表符
	\v	垂直制表符
	\xHH	（1到2位）十六进制值为 HH 的字节

就像 dd 命令一样，write 命令既不扩大也不缩小目标文件的文件大小，字符串的
剩余部分将被丢弃。目标文件也不能是一个压缩过的文件。

还是和 dd 类似，write 命令也具有危险性，使用风险你自己承担。作为一种安全
方面的考虑，你应当只向内存中的文件写入。

某些情况下当写入 NTFS 中的文件时，write命令可能失败。

假如你尝试在菜单中执行 write 命令来写入一个不是内存中的设备或者块文件时，
你会被安全的拒绝:-) （更新：现在不再限制）


!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!!!!
!!!!    Caution! The file to write can be a device name which stands      !!!!
!!!!    for all the sectors on the device. Take utmost care!              !!!!
!!!!______________________________________________________________________!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        警告！被write 命令写入的文件可以是一个设备名，即它代表了设备上的所有
        扇区。慎之又慎！


******************************************************************************
***                      为菜单项添加提示信息                              ***
******************************************************************************
                    
当你选择一个菜单项时，屏幕底部的提示信息将发生变化。

你可以在标题行中添加你的提示信息。必须用"\n" 开头，示例：
	title This is the title\nThis is the help text.\nAnd this is the 2nd line of the help text.

一些 C 语言风格的引用符号在请看前面章节的说明。

******************************************************************************
***        inird 命令可以为Linux 2.6 核心装载多个cpio 格式的文件           ***
******************************************************************************        
用法：
	initrd FILE [FILE ...]

注意 1：你不能用这种方法装载多于一个的老式磁盘镜像，因为Linux 核心不支持。
注意 2：其中的 FILE 必须和在syslinux中使用的顺序一样。

******************************************************************************
***               在固定位置访问一些内部变量                               ***
******************************************************************************

地址            长度            说明
=========	========	==============================================
0000:8208	4字节（即双字） 启动分区号 install_partition (the boot partition)
0000:8280	4字节（即双字） 启动驱动器号(boot_drive)
0000:8284	4字节（即双字） pxe 客户端 ip （即本地ip）
0000:8288	4字节（即双字） pxe 服务器 ip
0000:828C	4字节（即双字） pxe 网关 ip
0000:8290	8字节（即四字） 最后访问的文件的大小（是执行"cat --length=0"后的文件大小）
0000:8298	4字节（即双字） 从 1M 开始的连续内存块的大小（以 KB 为单位）
0000:829C	4字节（即双字） 当前根分区号(current root partition)
0000:82A0	4字节（即双字） 当前根所在的驱动器(current root drive)
0000:82A4	4字节（即双字） 解压标志 （gzip非自动解压）,非0时不自动解压
0000:82A8	8字节（即四字） 最后访问的分区的起始扇区号
0000:82B0	8字节（即四字） 最后访问的分区的扇区总数
0000:82B8	4字节（即双字） UD分区：磁头数，每磁道扇区数，真正驱动器号，最大每磁道扇区数（低位）
0000:8278	4字节（即双字)  GRUB4DOS编译的日期十进制数.
0000:82c0	8字节（即四字） 从 4G 开始的连续内存块的大小（以 KB 为单位）
=======

以下命令用于判断当前使用的GRUB是否在2010-12-30日编译的。
checkrange 20101230 read 0x8278

注意 1：Filesize 通过执行 "cat --length=0 FILE" 来初始化和修改。
注意 2：尽量不要改写这些变量（应该只是读取）。
注意 3：你可以使用内存地址6000:0000开始的 1K空间作为你自己的变量区（参见注意4）。
注意 4：read 命令现在从指定的地址处返回32位整数值。
注意 5：grub4dos 还没有变量扩展的功能。你只能使用整数变量。你不需要申明它们，就
        可以直接使用这些内存地址。通常你需要通过一个逻辑值或者一个条件测试命令
        来使用这些变量，即，类似这种格式："checkrange RANGE read ADDR"
注意 6：内部变量no_decompression, saved_drive and saved_partition 是可写的。

******************************************************************************
***            在 gfxmenu 之后后运行其它 menu.lst                          ***
******************************************************************************
                
注意下面是在 GFXMENU 之后使用 CONFIGILE 的示例：

	# The menu.lst file for gfxmenu
	default=0
	timeout=5
	gfxmenu /message
	configfile /another.lst
	title 0..........
	................
	title 1..........
	................
	title 2..........
	................
	# End of menu.lst

	# Begin another.lst
	default=0
	timeout=5
	title 0..........
	................
	title 1..........
	................
	title 2..........
	................
	# End of another.lst

会首先尝试执行 gfxmenu 命令。当它退出时（或者失败时）控制会转到 another.lst 菜单。

******************************************************************************
***                   a range of drives can be unmapped                    ***
******************************************************************************

用法：
	map --unmap=RANGE

其中的 RANGE 是一个已被映射的 BIOS 驱动器域。BIOS 驱动器号 0 表示第一软驱，1 表示
第二软驱；0x80 表示第一硬盘，0x81 表示第二硬盘，等等；虚拟光盘(hd32) 对应于
BIOS 驱动器号 0xA0 ,(hd33) 对应于0xA1 ，等等。

关于RANGE 的说明，请参阅前述的“新命令 CHECKRANGE ”这节。

示例 1：
	map --unmap=0,0x80,0xff

这将反映射虚拟软驱 (fd0),虚拟硬盘(hd0)和虚拟光盘(0xff)。

示例 2：
	map --unmap=0:0xff

这将反映射所有的虚拟软驱，所有的虚拟硬盘和所有的虚拟光盘。 

注意 1：通常，一条‘map’命令将在驱动器映射表中为虚拟驱动器增加一个表项。而
        ‘--unmap’意味着在驱动器映射表中（具体是指虚拟驱动器）的表项会被删除。

Note 2:	The --unhook option only breaks the INT13 hook(to the inerrupt
	vector table). It will not affect the drive map table. And later on
	execution of a `boot' command, the INT13 disk emulation routine will
	automatically get hooked(to the interrupt vector table) when needed
	(e.g., the drive map table is non-empty) even if it has been unhooked.
注意 2：--unhook 选项仅仅是断开 INT13 的挂钩（在中断矢量表中）。它不会影响到驱
        动器映射表。而且在执行了一个‘boot’命令之后，即使是它已经被反映射了的
        时候，INT13磁盘仿真程序也会在需要的时候（即，驱动器映射表非空时）自动建立挂钩。

注意 3：通常你需要在已经改变了驱动器映射表之后执行一条`map --rehook'命令。

******************************************************************************
***                         磁盘几何参数的修正和同步                       ***
******************************************************************************
                            
当一个USB 存储设备被连接到一台（或者是不同的）机器上时，分区表中或 BPB 中的磁盘
几何参数值可能是无效的，并且这个机器可能在启动时死机。因此你需要为驱动器找到一个
正确的磁盘几何参数（使用 `geometry --tune'），然后更新分区表或 BPB 中的磁盘
几何参数（使用`geometry --sync'）。

假如你想启动到DOS，那上面的步骤是必要的，因为 DOS 要求有正确的磁盘几何参数在分区
表和BPB 中。Windows 及 Linux 应该也需要，因为引导程序运行在实模式中。

******************************************************************************
***                            版本编号                                    ***
******************************************************************************

我们添加了一个字符 'a', 'b', 'c' or 'p' 到版本编号(e.g., 0.4.5).
所以现在版本编号是 0.4.5a, 0.4.5b, 0.4.5c, 0.4.5 or 0.4.5p.

'a' - alpha test. 不稳定, 尤其是在有已知BUG的情况下。
'b' - beta test. 测试版，开发人员觉得这个版本没有bug，希望有一个长期的测试。
'c' - 候选发布版，相对比较稳定。
''(nothing) - 正式版，比较稳定。
'p' - 修补版，对于在正式版中发现的一些问题进行修正.

******************************************************************************
***            Running User Programs（外部命令，供开发人员参考）           ***
******************************************************************************

从0.4.5起，用户可以自行编写程序以在GRUB4DOS中运行。
该可执行程序文件必须以8字节grub4dos EXEC签名结尾。
	0x05, 0x18, 0x05, 0x03, 0xBA, 0xA7, 0xBA, 0xBC

程序的入口点在文件头，和DOS的.com文件很像（但我们是32位的程序)。

注：因为使用了linux gcc的特性，所以程序只能在linux下使用gcc进行编译。

附上一个简单的echo.c源码，供参考。
/*================ begin echo.c ================*/

/*
 * 编译:			
 gcc -Wl,--build-id=none -m32 -mno-sse -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE echo.c -o echo.o

 * disassemble:			objdump -d echo.o
 * confirm no relocation:	readelf -r echo.o
 * generate executable:		objcopy -O binary echo.o echo
 * 经过这一步之后生成的echo文件就是可以在grub4dos中运行的程序。
 * and then the resultant echo will be grub4dos executable.
 */

/*
 * This is a simple ECHO command, running under grub4dos.
 */
#define sprintf ((int (*)(char *, const char *, ...))((*(int **)0x8300)[0]))
#define printf(...) sprintf(NULL, __VA_ARGS__)

int i = 0x66666666;	/* 这是必要的，看下面的注释。*/
/* gcc treat the following as data only if a global initialization like the
 * above line occurs.
 */

/* GRUB4DOS可执行程序结尾必须有以下8个字节（EXEC签名） */
asm(".long 0x03051805");
asm(".long 0xBCBAA7BA");
/* 感谢上帝, gcc 会把上面的8个字按兵不动放在最终程序的最后面。
 * 不要在这里插入其它任何代码.
 */

int main(char *arg,int flags)
{
	return printf("%s\n",arg);
}
/*================  end  echo.c ================*/

0x8300 是 grub4dos 系统函数(API)的入口点. 你可以在 asm.S 源码中找到它的定义.

目前可以使用的函数和变量:
	http://grubutils.googlecode.com/svn/trunk/src/include/grub4dos.h

******************************************************************************
***                      Map options added by Karyonix                     ***
******************************************************************************

注：boot-land.net网站已经改成 reboot.pro
map --add-mbt= option to be used with --mem. If =0 master boot track will not
	be added automatically.
	配合--mem 使用. 如果=0 则不会自动添加主引导磁道.
	说明：默认情况下把一个分区镜像map为一个硬盘时会自动添加一个主引导磁道.
	      使用该参数可以禁止GRUB4DOS自动添加。一般不需要使用这个参数。

map --top option to be used with --mem. map --mem will try to allocate memory
	at highest available address.
	配合--mem 使用. 如果=0 则不会自动添加主引导磁道.
	说明：默认情况下把一个分区镜像map为一个硬盘时会自动添加一个主引导磁道.
	      使用该参数可以禁止GRUB4DOS自动添加。一般不需要使用这个参数。
map --mem-max=, map --mem-min options to be used before map --mem. Allow user
	to manually limit range of address that map --mem can use.

safe_parse_maxint_with_suffix function parses K,M,G,T suffix after number.
注：更新的GRUB4DOS版本中已经使用这个参数替换了默认的safe_parse_maxint函数。
所以只要支持数值的命令行都可以使用以上特性。比如：
read 0x100000	//读取内存1MB处的数值
可以写成如下方式，方便使用。
read 1m
其它的命令只要支持数值输入的都可以使用这个特性。

******************************************************************************
***                 Graphics mode 6A: 800x600 with 16 colors               ***
******************************************************************************

现在有两2种可选的图形模式，默认的是640x480模式.
新的是800x600模式(对一些机子支持不是很好,有可能会死机）。

使用以下方法可以切换图形显示模式。

1. 确定目前是在控制台模式，你可以执行命令 "terminal console" 进行切换。
2. 使用命令"graphicsmode 0x6a" 设置图形模式为0x6A。
3. 进入图形模式，你可以使用命令"terminal graphics".
   如果在切换之前不是图形模式，那该命令无效，你可以使用splashimg或fontfile命令。

注： 1. 如果想换回默认的640x480，把上面的第2步改成"graphicsmode 0x12".
     2. 经过改进,更新的版本，可以直接改变，只要上面第2步一条命令就可以搞定。
     	例子：
     	在默认图形模式中(使用splashimage或fontfile命令都会进入图形模式）.
     	输入以下命令可以直接切换到800x600.
     		graphicsmode 0x6a

*****************************************************************************
*****			GRUB4DOS的变量支持				*****
*****************************************************************************

新的版本支持变量，用法和MSDOS一样。
关键命令：
	set [/p] [/a|/A] [/l|/u] [VARIABLE=[STRING]]

	variable  指定环境变量名（最长8个字符）。
	string    指定要指派给变量的一系列字符串（最长512个字符）。

	不带参数的 SET命令会显示当前变量。

	要删除某个变量，只需要让＝后面为空就可以
		set myvar=
	将会删除变量myvar

	显示已使用的名称的所有变量的值。例如:
		set ex_
	会显示所有以ex_开头的变量，如果没有任何匹配返回0.

注：1.使用和MSDOS一样的处理方用户法，一整行的命令会在执行前先进行变量替换。
    2.变量名必须使用字母或_开头。否则你将无法访问你的变量。
    3.长度限制请看前面说明。
    4.输入"set *"可以清除所有已设置的变量。
    5./a  后面的STRING是一个表达式，将调用CALC进行计算，保存结果为10进制数。
    6./A  同上，但保存结果为16进制数。
    7./l|/u 大小写转换。
    8./p  显示一个提示STRING并获取用户的输入内容并设置为变量VARIABLE的值。
    9.目前最多只能使用63个变量(实际上可使用60个),如果需要更多的变量可以通过以下方法扩展(最多可以扩展到0XFFFF个).
      set @extend BASE_ADDR SIZE
      BASE_ADDR 是用用户提供的一块可以安全使用的内存地址,SIZE要扩展的变量个数.
      使用 set @extend可以查看当前扩展的变量地址信息.
    10. 因为某些主板的原因会导致默认的变量被清空,为解决这一问题,新的版本允许修改默认变量的使用的内存位置.
        write 0x307FF4 0x45000
        0x45000是默认的变量存放位置,如果这个内存位置出现异常,请修改成其它地址.

新增的命令if
	if [/I] [NOT] STRING1==STRING2 [COMMAND]
	if [NOT] exist VARIABLE|FILENAME [COMMAND]
	1.如果STRING1==STRING2 字符串匹配，执行后面的COMMAND(如果有指定的话）。
	  否则返回TRUE。
	2./I 参数指写不区分大小写匹配。
	3.[NOT] 相反，如果STRING1==STRING2不匹配。
	4.exist 用于判断变量VARIABLE或文件FILENAME是否存在（filename必须以"/"或"("开头）.

	例子：
		1.判断字符串是否相等，并且不区分大小写。
		if /i test==%myvar% echo this is a test
		2.判断字符是否为空。
		if %myvar%#==# echo variable myvar not defined.
		注：我们使用了一个#不防止空操作，当然也可以使用其它字符，如
		if "%myvar%"=="" echo variable myvar not defined.

使用方法举例：
	1.显示一个包括变量的串。
		echo myvar = %myvar%
	2.使用一个变量代替命令。
		set print=echo
		%print% This a test.
	3.你可以使用一个“^”来阻此被变被扩展，例子
		echo %myvar^%
		或
		echo %my^var%
		将会显示 %myvar%而不是扩展myvar之后的字符。
	总之，只要出现了^那就不会扩展这个变量。

注：我们只处理在%%之间的^符号。

*****************************************************************************
*****			GRUB4DOS的批处理脚本支持			*****
*****************************************************************************

新的版本支持运行一个批处理脚本，语法和MS-DOS的批处理几乎一模一样。
你不需要学习新的知识就可以应用GRUB4DOS的批处理，唯一要做的就是学习GRUB4DOS命令。

例子一个简单的脚本（看一下是不是和MS-DOS一样）：
	=========GRUB4DOS BATCH SCRIPT START===============================
	!BAT #注：文件头!BAT是必须的用于识别这是一个GRUB4DOS批处理脚本
	echo %0
	echo Your type: %1 %2 %3 %4 %5 %6 %7 %8 %9
	call :label1 This is a test string
	goto :label2
	:label1
	echo %1 %2 %3 %4 %5 %6 %7 %8 %9
	goto :eof
	:label2
	echo end of batch script.
	=========GRUB4DOS BATCH SCRIPT END===============================

一些区别说明：
	1.出现错误时将停止执行。
	2.如果需要中途停止批处理脚本的运行可以用exit 1
	3.%9是指剩下的所有参数。
	4.支持shift命令。
	5.可扩展参数
		%~d0	扩展%0到磁盘号.例如：(hd0,0)，默认是()。
		%~p0	扩展%0到一个路径。
		%~n0	扩展%0到一个文件名.
		%~x0	扩展%0到一个文件扩展名。
		%~f0	扩展%0到一个完整的文件路径名(相当于%~dpnx0).
		%~z0	扩展%0到文件大小.
	6.其它用法请参考CMD的批处理。
在这里可以找到一些脚本
http://chenall.net/post/tag/grub4dos/

********************************************************************************
			条件菜单(iftitle)
********************************************************************************
自2011-12-04的版本开始支持条件菜单，可以根据某个特定的条件来决定是否显示某个菜单。
为了区别之前的普通菜单，使用新的参数iftitle。
语法如下:
	iftitle [<command>] Actual Title displayed\nOptional help line
	iftitle [<command>] 菜单标题\n菜单帮助

注意:
	1.command必须是一个合法的GRUB4DOS命令，支持调用外部命令。
	注：像echo/pause之类的命令在条件菜单命令中被禁用。
	    大部份的命令都可以使用，如果碰到不能使用的不要奇怪，非要使用可以提交BUG。
	2.菜单标题前至少要保留一个空格.
	3.[]是必须的，不可少。
	4.如果[]里面的内容为空相当于title即不判断。
	5.你可以使用该功能来快速注释整个菜单的内容（不显示菜单），只需要使用一个非法的命令即可。


******************************************************************************
***                           关于 usb2.0 驱动程序                         ***
******************************************************************************
grldr 内含 usb2.0 驱动。

usb2.0 驱动支持：PCI 分类代码的 0c/03/20，即 ehci（增强总线控制接口）设备。

usb2.0 驱动支持：usb（通用串行总线）的接口类型08（大容量存储类）、子类06、协议50，
即通常的u盘或移动硬盘。

支持 usb-fdd, usb-hdd, usb-cdrom 模式。 

在菜单或命令行加载 usb2.0 驱动：usb --init
可选参数：usb --delay=P    P是控制传输延时指数。0=常规；1=2*常规；2=4*常规；3=8*常规；

      
贴士：1. 有些 u 盘被 Windows 或 DOS 下的 usbaspi.sys 识别为 usb1.0 设备，但是
         增加延时后会重新识别为 usb2.0 设备。
      2. 有些 u 盘插入计算机前端插口不被识别，但是增加延时后会被识别。
      3. 加载失败时，选 --delay= 参数试一试。

2014-03-19
支持有碎片的文件仿真，最多 32 个片段。

*************************************************************************************
*                      批处理调试功能                                               *
*************************************************************************************

用法
debug PROG ARG

在调试模式下会进入单步模式,在每一行命令前等待用户按键有以下功能可以使用

Q->退出程序
C->进入命令行
S->跳过当前行
B->设置断点
E->停用调试,运行到程序结束或断点行.
N->运行到下一个函数的第一行

其它键直接执行当前行.

B->设置断点.可以使用的格式如下:

[*|+|-]INTEGER

默认情况下这个数值是一个绝对的行号.
前导`*`     后面的数值是一个内存地址.程序会先读取该处内存的值,执行的时候判断该内存的值是否有变化,有变化就中断.
前导`+`/`-` 后面的数值是一个相对行号.