/*
 * Copyright 2002-2008, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 *		Axel Dörfler, axeld@pinc-software.de
 */

/*!
	\file Mime.cpp
	Mime type C functions implementation.
*/

#include <errno.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <AutoDeleter.h>
#include <Bitmap.h>
#include <Drivers.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <fs_attr.h>
#include <fs_info.h>
#include <IconUtils.h>
#include <Mime.h>
#include <MimeType.h>
#include <mime/database_access.h>
#include <Node.h>
#include <Path.h>
#include <RegistrarDefs.h>
#include <Roster.h>
#include <RosterPrivate.h>

using namespace BPrivate;

enum {
	NOT_IMPLEMENTED	= B_ERROR,
};

// do_mime_update
//! Helper function that contacts the registrar for mime update calls
status_t
do_mime_update(int32 what, const char *path, int recursive,
	int synchronous, int force)
{
	BEntry root;
	entry_ref ref;

	status_t err = root.SetTo(path ? path : "/");
	if (!err)
		err = root.GetRef(&ref);
	if (!err) {
		BMessage msg(what);
		BMessage reply;
		status_t result;

		// Build and send the message, read the reply
		if (!err)
			err = msg.AddRef("entry", &ref);
		if (!err)
			err = msg.AddBool("recursive", recursive);
		if (!err)
			err = msg.AddBool("synchronous", synchronous);
		if (!err)
			err = msg.AddInt32("force", force);
		if (!err)
			err = BRoster::Private().SendTo(&msg, &reply, true);
		if (!err)
			err = reply.what == B_REG_RESULT ? B_OK : B_BAD_VALUE;
		if (!err)
			err = reply.FindInt32("result", &result);
		if (!err)
			err = result;
	}
	return err;
}

// update_mime_info
/*!	\brief Updates the MIME information (i.e MIME type) for one or more files.
	If \a path points to a file, the MIME information for this file are
	updated only. If it points to a directory and \a recursive is non-null,
	the information for all the files in the given directory tree are updated.
	If path is \c NULL all files are considered; \a recursive is ignored in
	this case.
	\param path The path to a file or directory, or \c NULL.
	\param recursive Non-null to trigger recursive behavior.
	\param synchronous If non-null update_mime_info() waits until the
		   operation is finished, otherwise it returns immediately and the
		   update is done asynchronously.
	\param force Specifies how to handle files that already have MIME
		   information:
			- \c B_UPDATE_MIME_INFO_NO_FORCE: Files that already have a
			  \c BEOS:TYPE attribute won't be updated.
			- \c B_UPDATE_MIME_INFO_FORCE_KEEP_TYPE: Files that already have a
			  \c BEOS:TYPE attribute will be updated too, but \c BEOS:TYPE
			  itself will remain untouched.
			- \c B_UPDATE_MIME_INFO_FORCE_UPDATE_ALL: Similar to
			  \c B_UPDATE_MIME_INFO_FORCE_KEEP_TYPE, but the \c BEOS:TYPE
			  attribute will be updated too.
	\return
	- \c B_OK: Everything went fine.
	- An error code otherwise.
*/
int
update_mime_info(const char *path, int recursive, int synchronous, int force)
{
	// Force recursion when given a NULL path
	if (!path)
		recursive = true;

	return do_mime_update(B_REG_MIME_UPDATE_MIME_INFO, path, recursive,
		synchronous, force);
}

// create_app_meta_mime
/*!	Creates a MIME database entry for one or more applications.
	If \a path points to an application file, a MIME DB entry is create for the
	application. If it points to a directory and \a recursive is non-null,
	entries are created for all application files in the given directory
	tree. If path is \c NULL all files are considered; \a recursive is
	ignored in this case.
	\param path The path to an application file, a directory, or \c NULL.
	\param recursive Non-null to trigger recursive behavior.
	\param synchronous If non-null create_app_meta_mime() waits until the
		   operation is finished, otherwise it returns immediately and the
		   operation is done asynchronously.
	\param force If non-null, entries are created even if they do already
		   exist.
	\return
	- \c B_OK: Everything went fine.
	- An error code otherwise.
*/
status_t
create_app_meta_mime(const char *path, int recursive, int synchronous,
	int force)
{
	// Force recursion when given a NULL path
	if (!path)
		recursive = true;

	return do_mime_update(B_REG_MIME_CREATE_APP_META_MIME, path, recursive,
		synchronous, force);
}


/*!	Retrieves an icon associated with a given device.
	\param dev The path to the device.
	\param icon A pointer to a buffer the icon data shall be written to.
	\param size The size of the icon. Currently the sizes 16 (small, i.e
	            \c B_MINI_ICON) and 32 (large, 	i.e. \c B_LARGE_ICON) are
	            supported.

	\todo The mounted directories for volumes can also have META:X:STD_ICON
		  attributes. Should those attributes override the icon returned
		  by ioctl(,B_GET_ICON,)?

	\return
	- \c B_OK: Everything went fine.
	- An error code otherwise.
*/
status_t
get_device_icon(const char *device, void *icon, int32 size)
{
	if (device == NULL || icon == NULL
		|| (size != B_LARGE_ICON && size != B_MINI_ICON))
		return B_BAD_VALUE;

	int fd = open(device, O_RDONLY);
	if (fd < 0)
		return errno;

	device_icon iconData = {size, icon};
	if (ioctl(fd, B_GET_ICON, &iconData) != 0) {
		// legacy icon was not available, try vector icon
		close(fd);

		uint8* data;
		size_t dataSize;
		type_code type;
		status_t status = get_device_icon(device, &data, &dataSize, &type);
		if (status == B_OK) {
			BBitmap* icon32 = new(std::nothrow) BBitmap(
				BRect(0, 0, size - 1, size - 1), B_BITMAP_NO_SERVER_LINK,
				B_RGBA32);
			BBitmap* icon8 = new(std::nothrow) BBitmap(
				BRect(0, 0, size - 1, size - 1), B_BITMAP_NO_SERVER_LINK,
				B_CMAP8);

			ArrayDeleter<uint8> dataDeleter(data);
			ObjectDeleter<BBitmap> icon32Deleter(icon32);
			ObjectDeleter<BBitmap> icon8Deleter(icon8);

			if (icon32 == NULL || icon32->InitCheck() != B_OK || icon8 == NULL
				|| icon8->InitCheck() != B_OK) {
				return B_NO_MEMORY;
			}

			status = BIconUtils::GetVectorIcon(data, dataSize, icon32);
			if (status == B_OK)
				status = BIconUtils::ConvertToCMAP8(icon32, icon8);
			if (status == B_OK)
				memcpy(icon, icon8->Bits(), icon8->BitsLength());

			return status;
		}
		return errno;
	}

	close(fd);
	return B_OK;
}


/*!	Retrieves an icon associated with a given device.
	\param dev The path to the device.
	\param icon A pointer to a pre-allocated BBitmap of the correct dimension
		   to store the requested icon (16x16 for the mini and 32x32 for the
		   large icon).
	\param which Specifies the size of the icon to be retrieved:
		   \c B_MINI_ICON for the mini and \c B_LARGE_ICON for the large icon.

	\todo The mounted directories for volumes can also have META:X:STD_ICON
		  attributes. Should those attributes override the icon returned
		  by ioctl(,B_GET_ICON,)?

	\return
	- \c B_OK: Everything went fine.
	- An error code otherwise.
*/
status_t
get_device_icon(const char *device, BBitmap *icon, icon_size which)
{
	// check parameters
	if (device == NULL || icon == NULL)
		return B_BAD_VALUE;

	uint8* data;
	size_t size;
	type_code type;
	status_t status = get_device_icon(device, &data, &size, &type);
	if (status == B_OK) {
		status = BIconUtils::GetVectorIcon(data, size, icon);
		delete[] data;
		return status;
	}

	// Vector icon was not available, try old one

	BRect rect;
	if (which == B_MINI_ICON)
		rect.Set(0, 0, 15, 15);
	else if (which == B_LARGE_ICON)
		rect.Set(0, 0, 31, 31);

	BBitmap* bitmap = icon;
	int32 iconSize = which;

	if (icon->ColorSpace() != B_CMAP8
		|| (which != B_MINI_ICON && which != B_LARGE_ICON)) {
		if (which < B_LARGE_ICON)
			iconSize = B_MINI_ICON;
		else
			iconSize = B_LARGE_ICON;

		bitmap = new(std::nothrow) BBitmap(
			BRect(0, 0, iconSize - 1, iconSize -1), B_BITMAP_NO_SERVER_LINK,
			B_CMAP8);
		if (bitmap == NULL || bitmap->InitCheck() != B_OK) {
			delete bitmap;
			return B_NO_MEMORY;
		}
	}

	// get the icon, convert temporary data into bitmap if necessary
	status = get_device_icon(device, bitmap->Bits(), iconSize);
	if (status == B_OK && icon != bitmap)
		status = BIconUtils::ConvertFromCMAP8(bitmap, icon);

	if (icon != bitmap)
		delete bitmap;

	return status;
}


status_t
get_device_icon(const char *device, uint8** _data, size_t* _size,
	type_code* _type)
{
	if (device == NULL || _data == NULL || _size == NULL || _type == NULL)
		return B_BAD_VALUE;

	int fd = open(device, O_RDONLY);
	if (fd < 0)
		return errno;

	// Try to get the icon by name first

	char name[B_FILE_NAME_LENGTH];
	if (ioctl(fd, B_GET_ICON_NAME, name) >= 0) {
		status_t status = get_named_icon(name, _data, _size, _type);
		if (status == B_OK) {
			close(fd);
			return B_OK;
		}
	}

	// Getting the named icon failed, try vector icon next

	// NOTE: The actual icon size is unknown as of yet. After the first call
	// to B_GET_VECTOR_ICON, the actual size is known and the final buffer
	// is allocated with the correct size. If the buffer needed to be
	// larger, then the temporary buffer above will not yet contain the
	// valid icon data. In that case, a second call to B_GET_VECTOR_ICON
	// retrieves it into the final buffer.
	uint8 data[8192];
	device_icon iconData = {sizeof(data), data};
	status_t status = ioctl(fd, B_GET_VECTOR_ICON, &iconData,
		sizeof(device_icon));
	if (status != 0)
		status = errno;

	if (status == B_OK) {
		*_data = new(std::nothrow) uint8[iconData.icon_size];
		if (*_data == NULL)
			status = B_NO_MEMORY;
	}

	if (status == B_OK) {
		if (iconData.icon_size > (int32)sizeof(data)) {
			// the stack buffer does not contain the data, see NOTE above
			iconData.icon_data = *_data;
			status = ioctl(fd, B_GET_VECTOR_ICON, &iconData,
				sizeof(device_icon));
			if (status != 0)
				status = errno;
		} else
			memcpy(*_data, data, iconData.icon_size);

		*_size = iconData.icon_size;
		*_type = B_VECTOR_ICON_TYPE;
	}

	// TODO: also support getting the old icon?
	close(fd);
	return status;
}


status_t
get_named_icon(const char* name, BBitmap* icon, icon_size which)
{
	// check parameters
	if (name == NULL || icon == NULL)
		return B_BAD_VALUE;

	BRect rect;
	if (which == B_MINI_ICON)
		rect.Set(0, 0, 15, 15);
	else if (which == B_LARGE_ICON)
		rect.Set(0, 0, 31, 31);
	else
		return B_BAD_VALUE;

	if (icon->Bounds() != rect)
		return B_BAD_VALUE;

	uint8* data;
	size_t size;
	type_code type;
	status_t status = get_named_icon(name, &data, &size, &type);
	if (status == B_OK)
		status = BIconUtils::GetVectorIcon(data, size, icon);

	delete[] data;
	return status;
}


status_t
get_named_icon(const char* name, uint8** _data, size_t* _size, type_code* _type)
{
	if (name == NULL || _data == NULL || _size == NULL || _type == NULL)
		return B_BAD_VALUE;

	directory_which kWhich[] = {
		B_USER_DATA_DIRECTORY,
		B_COMMON_DATA_DIRECTORY,
		B_BEOS_DATA_DIRECTORY,
	};

	status_t status = B_ENTRY_NOT_FOUND;
	BFile file;
	off_t size;

	for (uint32 i = 0; i < sizeof(kWhich) / sizeof(kWhich[0]); i++) {
		BPath path;
		if (find_directory(kWhich[i], &path) != B_OK)
			continue;

		path.Append("icons");
		path.Append(name);

		status = file.SetTo(path.Path(), B_READ_ONLY);
		if (status == B_OK) {
			status = file.GetSize(&size);
			if (size > 1024 * 1024)
				status = B_ERROR;
		}
		if (status == B_OK)
			break;
	}

	if (status != B_OK)
		return status;

	*_data = new(std::nothrow) uint8[size];
	if (*_data == NULL)
		return B_NO_MEMORY;

	if (file.Read(*_data, size) != size) {
		delete[] *_data;
		return B_ERROR;
	}

	*_size = size;
	*_type = B_VECTOR_ICON_TYPE;
		// TODO: for now

	return B_OK;
}