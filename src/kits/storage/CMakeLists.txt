target_include_directories(be PRIVATE
	"./"
	"mime"
	"disk_device"
	"disk_device/jobs"
)

target_sources(be PRIVATE
	AddOnImage.cpp
	AddOnMonitor.cpp
	AddOnMonitorHandler.cpp
	AppFileInfo.cpp
	CopyEngine.cpp
	Directory.cpp
	DriverSettings.cpp
	Entry.cpp
	EntryList.cpp
	EntryOperationEngineBase.cpp
	FdIO.cpp
	File.cpp
	FileDescriptorIO.cpp
	FileIO.cpp
	FindDirectory.cpp
	MergedDirectory.cpp
	Mime.cpp
	MimeType.cpp
	Node.cpp
	NodeInfo.cpp
	NodeMonitor.cpp
	NodeMonitorHandler.cpp
	OffsetFile.cpp
	Path.cpp
	PathFinder.cpp
	PathMonitor.cpp
	Query.cpp
	QueryPredicate.cpp
	RemoveEngine.cpp
	ResourceFile.cpp
	ResourceItem.cpp
	ResourcesContainer.cpp
	Resources.cpp
	ResourceStrings.cpp
	Statable.cpp
	storage_support.cpp
	SymLink.cpp
	Volume.cpp
	VolumeRoster.cpp

	mime/AppMetaMimeCreator.cpp
	mime/AssociatedTypes.cpp
	mime/Database.cpp
	mime/DatabaseDirectory.cpp
	mime/DatabaseLocation.cpp
	mime/database_support.cpp
	mime/InstalledTypes.cpp
	mime/MimeEntryProcessor.cpp
	mime/MimeInfoUpdater.cpp
	mime/MimeSnifferAddon.cpp
	mime/MimeSnifferAddonManager.cpp
	mime/MimeSniffer.cpp
	mime/SnifferRules.cpp
	mime/Supertype.cpp
	mime/SupportingApps.cpp
	mime/TextSnifferAddon.cpp

	sniffer/CharStream.cpp
	sniffer/DisjList.cpp
	sniffer/Err.cpp
	sniffer/Parser.cpp
	sniffer/Pattern.cpp
	sniffer/PatternList.cpp
	sniffer/Range.cpp
	sniffer/RPattern.cpp
	sniffer/RPatternList.cpp
	sniffer/Rule.cpp

	disk_device/DiskDevice.cpp
	disk_device/DiskDeviceJob.cpp
	disk_device/DiskDeviceJobGenerator.cpp
	disk_device/DiskDeviceJobQueue.cpp
	disk_device/DiskDeviceList.cpp
	disk_device/DiskDevicePrivate.cpp
	disk_device/DiskDeviceRoster.cpp
	disk_device/DiskDeviceTypes.cpp
	disk_device/DiskDeviceVisitor.cpp
	disk_device/DiskScannerAddOn.cpp
	disk_device/DiskSystemAddOn.cpp
	disk_device/DiskSystemAddOnManager.cpp
	disk_device/DiskSystem.cpp
	disk_device/MutablePartition.cpp
	disk_device/Partition.cpp
	disk_device/PartitionDelegate.cpp
	disk_device/PartitioningDialog.cpp
	disk_device/PartitioningInfo.cpp
	disk_device/PartitionParameterEditor.cpp
	disk_device/PartitionReference.cpp

	disk_device/jobs/CreateChildJob.cpp
	disk_device/jobs/RepairJob.cpp
	disk_device/jobs/DefragmentJob.cpp
	disk_device/jobs/ResizeJob.cpp
	disk_device/jobs/DeleteChildJob.cpp
	disk_device/jobs/SetStringJob.cpp
	disk_device/jobs/InitializeJob.cpp
	disk_device/jobs/UninitializeJob.cpp
	disk_device/jobs/MoveJob.cpp
)
