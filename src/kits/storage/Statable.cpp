/*
 * Copyright 2002-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 */

/*!
	\file Statable.cpp
	BStatable implementation.
*/

#include <Statable.h>

#include <sys/stat.h>

#include <compat/sys/stat.h>

#include <Node.h>
#include <NodeMonitor.h>
#include <Volume.h>


class BStatable::Private {
public:
	Private(const BStatable* object)
		:
		fObject(object)
	{
	}

	status_t GetStatBeOS(struct stat_beos* st)
	{
		return fObject->_GetStat(st);
	}

private:
	const BStatable*	fObject;
};


#if __GNUC__ > 3
BStatable::~BStatable()
{
}
#endif


/*!	\fn status_t GetStat(struct stat *st) const
	\brief Returns the stat stucture for the node.
	\param st the stat structure to be filled in.
	\return
	- \c B_OK: Worked fine
	- \c B_NO_MEMORY: Could not allocate the memory for the call.
	- \c B_BAD_VALUE: The current node does not exist.
	- \c B_NOT_ALLOWED: Read only node or volume.
*/


/*!	\brief Returns if the current node is a file.
	\return \c true, if the BNode is properly initialized and is a file,
			\c false otherwise.
*/
bool
BStatable::IsFile() const
{
	struct stat statData;
	if (GetStat(&statData) == B_OK)
		return S_ISREG(statData.st_mode);
	else
		return false;
}

/*!	\brief Returns if the current node is a directory.
	\return \c true, if the BNode is properly initialized and is a file,
			\c false otherwise.
*/
bool
BStatable::IsDirectory() const
{
	struct stat statData;
	if (GetStat(&statData) == B_OK)
		return S_ISDIR(statData.st_mode);
	else
		return false;
}

/*!	\brief Returns if the current node is a symbolic link.
	\return \c true, if the BNode is properly initialized and is a symlink,
			\c false otherwise.
*/
bool
BStatable::IsSymLink() const
{
	struct stat statData;
	if (GetStat(&statData) == B_OK)
		return S_ISLNK(statData.st_mode);
	else
		return false;
}

/*!	\brief Returns a node_ref for the current node.
	\param ref the node_ref structure to be filled in
	\see GetStat() for return codes
*/
status_t
BStatable::GetNodeRef(node_ref *ref) const
{
	status_t error = (ref ? B_OK : B_BAD_VALUE);
	struct stat statData;
	if (error == B_OK)
		error = GetStat(&statData);
	if (error == B_OK) {
		ref->device  = statData.st_dev;
		ref->node = statData.st_ino;
	}
	return error;
}

/*!	\brief Returns the owner of the node.
	\param owner a pointer to a uid_t variable to be set to the result
	\see GetStat() for return codes
*/
status_t
BStatable::GetOwner(uid_t *owner) const
{
	status_t error = (owner ? B_OK : B_BAD_VALUE);
	struct stat statData;
	if (error == B_OK)
		error = GetStat(&statData);
	if (error == B_OK)
		*owner = statData.st_uid;
	return error;
}

/*!	\brief Sets the owner of the node.
	\param owner the new owner
	\see GetStat() for return codes
*/
status_t
BStatable::SetOwner(uid_t owner)
{
	struct stat statData;
	statData.st_uid = owner;
	return set_stat(statData, B_STAT_UID);
}

/*!	\brief Returns the group owner of the node.
	\param group a pointer to a gid_t variable to be set to the result
	\see GetStat() for return codes
*/
status_t
BStatable::GetGroup(gid_t *group) const
{
	status_t error = (group ? B_OK : B_BAD_VALUE);
	struct stat statData;
	if (error == B_OK)
		error = GetStat(&statData);
	if (error == B_OK)
		*group = statData.st_gid;
	return error;
}

/*!	\brief Sets the group owner of the node.
	\param group the new group
	\see GetStat() for return codes
*/
status_t
BStatable::SetGroup(gid_t group)
{
	struct stat statData;
	statData.st_gid = group;
	return set_stat(statData, B_STAT_GID);
}

/*!	\brief Returns the permissions of the node.
	\param perms a pointer to a mode_t variable to be set to the result
	\see GetStat() for return codes
*/
status_t
BStatable::GetPermissions(mode_t *perms) const
{
	status_t error = (perms ? B_OK : B_BAD_VALUE);
	struct stat statData;
	if (error == B_OK)
		error = GetStat(&statData);
	if (error == B_OK)
		*perms = (statData.st_mode & S_IUMSK);
	return error;
}

/*!	\brief Sets the permissions of the node.
	\param perms the new permissions
	\see GetStat() for return codes
*/
status_t
BStatable::SetPermissions(mode_t perms)
{
	struct stat statData;
	// the FS should do the correct masking -- only the S_IUMSK part is
	// modifiable
	statData.st_mode = perms;
	return set_stat(statData, B_STAT_MODE);
}

/*!	\brief Get the size of the node's data (not counting attributes).
	\param size a pointer to a variable to be set to the result
	\see GetStat() for return codes
*/
status_t
BStatable::GetSize(off_t *size) const
{
	status_t error = (size ? B_OK : B_BAD_VALUE);
	struct stat statData;
	if (error == B_OK)
		error = GetStat(&statData);
	if (error == B_OK)
		*size = statData.st_size;
	return error;
}

/*!	\brief Returns the last time the node was modified.
	\param mtime a pointer to a variable to be set to the result
	\see GetStat() for return codes
*/
status_t
BStatable::GetModificationTime(time_t *mtime) const
{
	status_t error = (mtime ? B_OK : B_BAD_VALUE);
	struct stat statData;
	if (error == B_OK)
		error = GetStat(&statData);
	if (error == B_OK)
		*mtime = statData.st_mtime;
	return error;
}

/*!	\brief Sets the last time the node was modified.
	\param mtime the new modification time
	\see GetStat() for return codes
*/
status_t
BStatable::SetModificationTime(time_t mtime)
{
	struct stat statData;
	statData.st_mtime = mtime;
	return set_stat(statData, B_STAT_MODIFICATION_TIME);
}

/*!	\brief Returns the time the node was created.
	\param ctime a pointer to a variable to be set to the result
	\see GetStat() for return codes
*/
status_t
BStatable::GetCreationTime(time_t *ctime) const
{
	#ifdef __HAIKU__
	status_t error = (ctime ? B_OK : B_BAD_VALUE);
	struct stat statData;
	if (error == B_OK)
		error = GetStat(&statData);
	if (error == B_OK)
		*ctime = statData.st_crtime;
	return error;
	#elseif
	printf("GetCreationTime UNIMPLEMENTED\n");
	return B_ERROR;
	#endif
}

/*!	\brief Sets the time the node was created.
	\param ctime the new creation time
	\see GetStat() for return codes
*/
status_t
BStatable::SetCreationTime(time_t ctime)
{
	#ifdef __HAIKU__
	struct stat statData;
	statData.st_crtime = ctime;
	return set_stat(statData, B_STAT_CREATION_TIME);
	#elseif
	printf("GetCreationTime UNIMPLEMENTED\n");
	return B_ERROR;
	#endif
}

/*!	\brief Returns the time the node was accessed.
	Not used.
	\see GetModificationTime()
	\see GetStat() for return codes
*/
status_t
BStatable::GetAccessTime(time_t *atime) const
{
	status_t error = (atime ? B_OK : B_BAD_VALUE);
	struct stat statData;
	if (error == B_OK)
		error = GetStat(&statData);
	if (error == B_OK)
		*atime = statData.st_atime;
	return error;
}

/*!	\brief Sets the time the node was accessed.
	Not used.
	\see GetModificationTime()
	\see GetStat() for return codes
*/
status_t
BStatable::SetAccessTime(time_t atime)
{
	struct stat statData;
	statData.st_atime = atime;
	return set_stat(statData, B_STAT_ACCESS_TIME);
}

/*!	\brief Returns the volume the node lives on.
	\param vol a pointer to a variable to be set to the result
	\see BVolume
	\see GetStat() for return codes
*/
status_t
BStatable::GetVolume(BVolume *vol) const
{
	status_t error = (vol ? B_OK : B_BAD_VALUE);
	struct stat statData;
	if (error == B_OK)
		error = GetStat(&statData);
	if (error == B_OK)
		error = vol->SetTo(statData.st_dev);
	return error;
}


// _OhSoStatable1() -> GetStat()
extern "C" status_t
#if __GNUC__ == 2
_OhSoStatable1__9BStatable(const BStatable *self, struct stat *st)
#else
_ZN9BStatable14_OhSoStatable1Ev(const BStatable *self, struct stat *st)
#endif
{
	// No Perform() method -- we have to use the old GetStat() method instead.
	struct stat_beos oldStat;
	status_t error = BStatable::Private(self).GetStatBeOS(&oldStat);
	if (error != B_OK)
		return error;

	convert_from_stat_beos(&oldStat, st);
	return B_OK;
}


void BStatable::_OhSoStatable2() {}
void BStatable::_OhSoStatable3() {}
