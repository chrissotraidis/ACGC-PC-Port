#import <AppKit/AppKit.h>

#include "pc_apple.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int pc_apple_choose_disc(char* out_path, size_t capacity) {
    if (out_path == NULL || capacity == 0) return 0;
    out_path[0] = '\0';

    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        panel.title = @"Choose your Animal Crossing disc image";
        panel.message = @"Bellpad supports a legally obtained GAFE01 revision 0 ISO, GCM, or CISO image.";
        panel.prompt = @"Use Disc Image";
        panel.canChooseFiles = YES;
        panel.canChooseDirectories = NO;
        panel.allowsMultipleSelection = NO;
        panel.allowedFileTypes = @[@"iso", @"gcm", @"ciso"];

        if ([panel runModal] != NSModalResponseOK) return 0;

        const char* path = panel.URL.fileSystemRepresentation;
        if (path == NULL || strlen(path) >= capacity) return 0;
        strcpy(out_path, path);
        return 1;
    }
}

int pc_apple_resource_path(const char* relative_path, char* out_path,
                           size_t capacity) {
    if (relative_path == NULL || out_path == NULL || capacity == 0) return 0;
    out_path[0] = '\0';

    @autoreleasepool {
        NSString* resource_root = NSBundle.mainBundle.resourcePath;
        if (resource_root == nil) return 0;

        NSString* relative = [NSString stringWithUTF8String:relative_path];
        if (relative == nil) return 0;
        NSString* full_path = [resource_root stringByAppendingPathComponent:relative];
        const char* file_path = full_path.fileSystemRepresentation;
        if (file_path == NULL || strlen(file_path) >= capacity) return 0;
        strcpy(out_path, file_path);
        return 1;
    }
}

int pc_apple_prepare_data_directory(void) {
    @autoreleasepool {
        NSString* data_path = nil;
        const char* override_path = getenv("BELLPAD_DATA_HOME");

        if (override_path != NULL && override_path[0] != '\0') {
            if (override_path[0] != '/') return 0;
            data_path = [NSString stringWithUTF8String:override_path];
        } else {
            NSArray<NSURL*>* roots = [NSFileManager.defaultManager
                URLsForDirectory:NSApplicationSupportDirectory
                inDomains:NSUserDomainMask];
            NSURL* root = roots.firstObject;
            data_path = [[root URLByAppendingPathComponent:@"Bellpad"
                                                isDirectory:YES] path];
        }

        if (data_path == nil) return 0;

        NSError* error = nil;
        if (![NSFileManager.defaultManager createDirectoryAtPath:data_path
                                     withIntermediateDirectories:YES
                                                      attributes:nil
                                                           error:&error]) {
            return 0;
        }

        const char* file_path = data_path.fileSystemRepresentation;
        return file_path != NULL && chdir(file_path) == 0;
    }
}

/* Relative to the prepared data directory, like settings.ini and save/. */
static NSString* const kDiscReferenceFile = @"disc.alias";

int pc_apple_store_disc_path(const char* path) {
    if (path == NULL || path[0] == '\0') return 0;

    @autoreleasepool {
        NSURL* url = [NSURL fileURLWithFileSystemRepresentation:path
                                                    isDirectory:NO
                                                  relativeToURL:nil];
        if (url == nil) return 0;

        NSData* reference = [url bookmarkDataWithOptions:0
                          includingResourceValuesForKeys:nil
                                           relativeToURL:nil
                                                   error:NULL];
        if (reference == nil) return 0;

        return [reference writeToFile:kDiscReferenceFile
                              options:NSDataWritingAtomic
                                error:NULL] ? 1 : 0;
    }
}

int pc_apple_load_disc_path(char* out_path, size_t capacity) {
    if (out_path == NULL || capacity == 0) return 0;
    out_path[0] = '\0';

    @autoreleasepool {
        NSData* reference = [NSData dataWithContentsOfFile:kDiscReferenceFile];
        if (reference == nil) return 0;

        BOOL stale = NO;
        NSURL* url = [NSURL URLByResolvingBookmarkData:reference
                                               options:NSURLBookmarkResolutionWithoutUI
                                         relativeToURL:nil
                                   bookmarkDataIsStale:&stale
                                                 error:NULL];
        if (url == nil) return 0;

        const char* resolved = url.fileSystemRepresentation;
        if (resolved == NULL || strlen(resolved) >= capacity) return 0;
        strcpy(out_path, resolved);

        /* The image moved; rewrite the reference from its current location. */
        if (stale) pc_apple_store_disc_path(resolved);
        return 1;
    }
}

void pc_apple_forget_disc_path(void) {
    @autoreleasepool {
        [NSFileManager.defaultManager removeItemAtPath:kDiscReferenceFile
                                                 error:NULL];
    }
}
