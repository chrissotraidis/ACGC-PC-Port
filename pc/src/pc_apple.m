#import <AppKit/AppKit.h>

#include "pc_apple.h"

#include <string.h>

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
