This readme file describes known problems and limitations with release 1.2 of the Fujitsu Softek TDMF software for Windows NT.


**Sustained I/O Rate Exceeds Network Bandwidth**

The Softek TDMF product is sensitive in circumstances where the write I/O rate on the primary system exceeds the network bandwidth to the secondary system for a sustained period of time. This problem applies only to asynchronous replication. 

If the problem occurs, the secondary system can get significantly behind the primary system. Softek TDMF will eventually recover, but no messages are provided to indicate how far behind the secondary system may be at any given instant.

To avoid this problem, make sure the write I/O rate on the primary system does not exceed the bandwidth of the network connection to the secondary system.


**Adding and Changing Storage Devices**

Drive letter assignments are controlled by the operating system in Windows NT 4.0 and Windows 2000. If a drive is added, deleted, or moved, the drive letters can change. If the drive letters change, Softek TDMF detects this and issues an error stating that you need to use Configuration Tool to re-assign each logical group’s local/mirror partition pairs to the correct drive letters. (See the section, "Working with Local/Mirror Partition Pairs" in the Softek TDMF Block Administrator’s Guide, Release 1.2.x, Windows NT/ Windows 2000 Version.)


**Integration with Application Failover Products**

To provide full server and application failover capabilities, Softek TDMF must be combined with other products (for example, Microsoft Cluster Service). However, the current schedule for release 1.2 does not include testing with any of these products. It is likely that product changes will have to be made to accommodate this integration.


**IDE Drives**

Softek TDMF does not support IDE drives. It is recommended that Softek TDMF be used only with drives that support integral bad sector sparing.


**Application Starting Order**

During system startup, applications that start prior to Softek TDMF can make changes to the primary storage. These changes are not detected by Softek TDMF and thus, are not replicated. This limitation has been encountered with SQL Server and is possible with other applications that begin accessing their storage before Softek TDMF is started. 

To avoid this problem, ensure that Softek TDMF is started before any application that accesses storage under Softek TDMF’s control. To do this, change the load order of dependent services in the Windows NT/Windows 2000 registry.


**Secondary Storage Security**

During a checkpoint operation, the secondary system’s partitions can be accessed by applications, which allows the secondary data to be changed.

After the checkpoint operation, you must determine what modifications have been made, and decide whether to propagate these changes back to the primary system, or to assume that the data on the primary system is correct and lose any changes.

You can do any of the following:

- Release the checkpoint and do nothing, in which case the primary and secondary systems are no longer in sync.

- Perform a backfresh operation, which copies attributes and changes from the secondary to the primary system. (See the description of the tdmflaunchbackfresh command in the "Commands" chapter in the Fujitsu Softek TDMF Block Administrator’s Guide, Release 1.2.x, Windows NT/Windows 2000 Version.)

- Perform a checksum refresh, which will synchronize the systems but lose all attribute sets. (See the description of the tdmflaunchrefresh command in the "Commands" chapter in the Softek TDMF Block Administrator’s Guide, Release 1.2.x, Windows NT/Windows 2000 Version.)


**Command Usage on Primary and Secondary Systems**

Many of the Softek TDMF commands must be run on the primary system; a few must be run on the secondary. If your primary and secondary systems are in different locations (for example, in different buildings), you may want to consider installing the Windows NT 4/ Windows 2000 Resource Kit, which will allow you to run any Softek TDMF command from either system.

To install the Resource Kit, do the following:

1. On both primary and secondary systems, install the Windows NT 4/Windows 2000 Resource Kit with addendum 4.

2. On both systems, install "Remote Console Server" using the rsetup command in the Windows NT/Windows 2000 Resource Kit installation directory.

3. On the primary system, install "Remote Command Server."

4. On the primary system, add Softek TDMF's install path to the Path variable (this is done through the Environment tab in System Properties).

5. Reboot both systems.

After you install the Windows NT 4/Windows 2000 Resource Kit, you can run commands remotely from an MSDOS command window, or you can execute commands through batch files, such as the one shown in the section, "Running Softek TDMF Commands from Either System" in the Administration chapter in the Administration Guide.

See the Windows NT 4/Windows 20000 Resource Kit documentation for more detailed information about installing the kit and executing commands remotely.

************************************
Modified Octobre 5th, 2001 10:03 AM

