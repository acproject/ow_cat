#include "platform/macos/macos_ime_adapter.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#import <InputMethodKit/InputMethodKit.h>
#import <Carbon/Carbon.h>
#import <Security/Security.h>
#import <ServiceManagement/ServiceManagement.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/host_info.h>
#include <unistd.h>
#include <pwd.h>
#endif

// C++ Implementation
struct MacOSSystemIntegration::Impl {
    // State
    bool isInitialized;
    std::string bundleIdentifier;
    std::string imeName;
    
    Impl() : isInitialized(false) {
        bundleIdentifier = "com.owcat.ime";
        imeName = "OwCat IME";
    }
};

MacOSSystemIntegration::MacOSSystemIntegration() : pImpl(std::make_unique<Impl>()) {
}

MacOSSystemIntegration::~MacOSSystemIntegration() = default;

bool MacOSSystemIntegration::initialize() {
#ifdef __APPLE__
    pImpl->isInitialized = true;
    return true;
#else
    return false;
#endif
}

void MacOSSystemIntegration::shutdown() {
    pImpl->isInitialized = false;
}

bool MacOSSystemIntegration::registerIME(const std::string& bundleId, const std::string& name) {
#ifdef __APPLE__
    pImpl->bundleIdentifier = bundleId;
    pImpl->imeName = name;
    
    // Get the main bundle
    NSBundle* mainBundle = [NSBundle mainBundle];
    if (!mainBundle) {
        std::cerr << "Failed to get main bundle" << std::endl;
        return false;
    }
    
    // Check if bundle identifier matches
    NSString* currentBundleId = [mainBundle bundleIdentifier];
    NSString* targetBundleId = [NSString stringWithUTF8String:bundleId.c_str()];
    
    if (![currentBundleId isEqualToString:targetBundleId]) {
        std::cerr << "Bundle identifier mismatch: " << [currentBundleId UTF8String] 
                  << " vs " << bundleId << std::endl;
        // This is not necessarily an error in development
    }
    
    // Register with TISInputSourceManager
    NSString* bundlePath = [mainBundle bundlePath];
    NSURL* bundleURL = [NSURL fileURLWithPath:bundlePath];
    
    OSStatus status = TISRegisterInputSource((__bridge CFURLRef)bundleURL);
    if (status != noErr) {
        std::cerr << "Failed to register input source: " << status << std::endl;
        return false;
    }
    
    std::cout << "Successfully registered IME: " << name << " (" << bundleId << ")" << std::endl;
    return true;
#else
    return false;
#endif
}

bool MacOSSystemIntegration::unregisterIME(const std::string& bundleId) {
#ifdef __APPLE__
    // Find the input source
    NSString* bundleIdStr = [NSString stringWithUTF8String:bundleId.c_str()];
    
    CFArrayRef inputSources = TISCreateInputSourceList(NULL, false);
    if (!inputSources) {
        return false;
    }
    
    bool found = false;
    CFIndex count = CFArrayGetCount(inputSources);
    
    for (CFIndex i = 0; i < count; i++) {
        TISInputSourceRef inputSource = (TISInputSourceRef)CFArrayGetValueAtIndex(inputSources, i);
        
        CFStringRef sourceId = (CFStringRef)TISGetInputSourceProperty(inputSource, kTISPropertyBundleID);
        if (sourceId && CFStringCompare(sourceId, (__bridge CFStringRef)bundleIdStr, 0) == kCFCompareEqualTo) {
            OSStatus status = TISDeregisterInputSource(inputSource);
            if (status == noErr) {
                found = true;
                std::cout << "Successfully unregistered IME: " << bundleId << std::endl;
            } else {
                std::cerr << "Failed to unregister input source: " << status << std::endl;
            }
            break;
        }
    }
    
    CFRelease(inputSources);
    return found;
#else
    return false;
#endif
}

bool MacOSSystemIntegration::checkAccessibilityPermissions() {
#ifdef __APPLE__
    // Check if accessibility permissions are granted
    NSDictionary* options = @{(__bridge id)kAXTrustedCheckOptionPrompt: @YES};
    Boolean isTrusted = AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);
    
    return isTrusted == TRUE;
#else
    return false;
#endif
}

bool MacOSSystemIntegration::requestAccessibilityPermissions() {
#ifdef __APPLE__
    // Request accessibility permissions with prompt
    NSDictionary* options = @{(__bridge id)kAXTrustedCheckOptionPrompt: @YES};
    Boolean isTrusted = AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);
    
    if (!isTrusted) {
        // Show alert to guide user
        NSAlert* alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Accessibility Permission Required"];
        [alert setInformativeText:@"OwCat IME needs accessibility permission to function properly. Please grant permission in System Preferences > Security & Privacy > Privacy > Accessibility."];
        [alert addButtonWithTitle:@"Open System Preferences"];
        [alert addButtonWithTitle:@"Cancel"];
        
        NSModalResponse response = [alert runModal];
        if (response == NSAlertFirstButtonReturn) {
            // Open System Preferences
            [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility"]];
        }
    }
    
    return isTrusted == TRUE;
#else
    return false;
#endif
}

std::string MacOSSystemIntegration::getSystemVersion() {
#ifdef __APPLE__
    NSProcessInfo* processInfo = [NSProcessInfo processInfo];
    NSOperatingSystemVersion version = [processInfo operatingSystemVersion];
    
    std::ostringstream oss;
    oss << "macOS " << version.majorVersion << "." << version.minorVersion;
    if (version.patchVersion > 0) {
        oss << "." << version.patchVersion;
    }
    
    return oss.str();
#else
    return "Unknown";
#endif
}

std::map<std::string, std::string> MacOSSystemIntegration::getSystemInfo() {
    std::map<std::string, std::string> info;
    
#ifdef __APPLE__
    // Operating System
    info["os_name"] = "macOS";
    info["os_version"] = getSystemVersion();
    
    // Architecture
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        info["architecture"] = unameData.machine;
        info["kernel_version"] = unameData.release;
    }
    
    // Processor information
    size_t size = 0;
    sysctlbyname("machdep.cpu.brand_string", NULL, &size, NULL, 0);
    if (size > 0) {
        char* cpu_brand = new char[size];
        if (sysctlbyname("machdep.cpu.brand_string", cpu_brand, &size, NULL, 0) == 0) {
            info["processor"] = cpu_brand;
        }
        delete[] cpu_brand;
    }
    
    // Processor count
    int processor_count = 0;
    size = sizeof(processor_count);
    if (sysctlbyname("hw.ncpu", &processor_count, &size, NULL, 0) == 0) {
        info["processor_count"] = std::to_string(processor_count);
    }
    
    // Memory information
    vm_size_t page_size;
    vm_statistics64_data_t vm_stat;
    mach_msg_type_number_t host_size = sizeof(vm_statistics64_data_t) / sizeof(natural_t);
    
    if (host_page_size(mach_host_self(), &page_size) == KERN_SUCCESS &&
        host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vm_stat, &host_size) == KERN_SUCCESS) {
        
        uint64_t total_memory = (vm_stat.free_count + vm_stat.active_count + vm_stat.inactive_count + 
                                vm_stat.wire_count) * page_size;
        uint64_t free_memory = vm_stat.free_count * page_size;
        uint64_t used_memory = total_memory - free_memory;
        
        info["total_memory"] = std::to_string(total_memory / (1024 * 1024)) + " MB";
        info["used_memory"] = std::to_string(used_memory / (1024 * 1024)) + " MB";
        info["free_memory"] = std::to_string(free_memory / (1024 * 1024)) + " MB";
    }
    
    // User information
    struct passwd* pw = getpwuid(getuid());
    if (pw) {
        info["username"] = pw->pw_name;
        info["home_directory"] = pw->pw_dir;
    }
    
    // Computer name
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        info["computer_name"] = hostname;
    }
    
    // Bundle information
    NSBundle* mainBundle = [NSBundle mainBundle];
    if (mainBundle) {
        NSString* bundleId = [mainBundle bundleIdentifier];
        NSString* version = [mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
        NSString* buildNumber = [mainBundle objectForInfoDictionaryKey:@"CFBundleVersion"];
        
        if (bundleId) info["bundle_id"] = [bundleId UTF8String];
        if (version) info["app_version"] = [version UTF8String];
        if (buildNumber) info["build_number"] = [buildNumber UTF8String];
    }
    
#else
    info["os_name"] = "Unknown";
    info["error"] = "Not compiled for macOS";
#endif
    
    return info;
}

std::vector<std::string> MacOSSystemIntegration::getInstalledIMEs() {
    std::vector<std::string> imes;
    
#ifdef __APPLE__
    CFArrayRef inputSources = TISCreateInputSourceList(NULL, false);
    if (!inputSources) {
        return imes;
    }
    
    CFIndex count = CFArrayGetCount(inputSources);
    
    for (CFIndex i = 0; i < count; i++) {
        TISInputSourceRef inputSource = (TISInputSourceRef)CFArrayGetValueAtIndex(inputSources, i);
        
        // Get input source category
        CFStringRef category = (CFStringRef)TISGetInputSourceProperty(inputSource, kTISPropertyInputSourceCategory);
        if (!category || CFStringCompare(category, kTISCategoryKeyboardInputSource, 0) != kCFCompareEqualTo) {
            continue;
        }
        
        // Get bundle ID
        CFStringRef bundleId = (CFStringRef)TISGetInputSourceProperty(inputSource, kTISPropertyBundleID);
        if (bundleId) {
            NSString* bundleIdStr = (__bridge NSString*)bundleId;
            imes.push_back([bundleIdStr UTF8String]);
        }
    }
    
    CFRelease(inputSources);
#endif
    
    return imes;
}

bool MacOSSystemIntegration::isIMEInstalled(const std::string& bundleId) {
#ifdef __APPLE__
    NSString* targetBundleId = [NSString stringWithUTF8String:bundleId.c_str()];
    
    CFArrayRef inputSources = TISCreateInputSourceList(NULL, false);
    if (!inputSources) {
        return false;
    }
    
    bool found = false;
    CFIndex count = CFArrayGetCount(inputSources);
    
    for (CFIndex i = 0; i < count; i++) {
        TISInputSourceRef inputSource = (TISInputSourceRef)CFArrayGetValueAtIndex(inputSources, i);
        
        CFStringRef sourceId = (CFStringRef)TISGetInputSourceProperty(inputSource, kTISPropertyBundleID);
        if (sourceId && CFStringCompare(sourceId, (__bridge CFStringRef)targetBundleId, 0) == kCFCompareEqualTo) {
            found = true;
            break;
        }
    }
    
    CFRelease(inputSources);
    return found;
#else
    return false;
#endif
}

std::string MacOSSystemIntegration::getCurrentIME() {
#ifdef __APPLE__
    TISInputSourceRef currentSource = TISCopyCurrentKeyboardInputSource();
    if (!currentSource) {
        return "";
    }
    
    CFStringRef bundleId = (CFStringRef)TISGetInputSourceProperty(currentSource, kTISPropertyBundleID);
    std::string result;
    
    if (bundleId) {
        NSString* bundleIdStr = (__bridge NSString*)bundleId;
        result = [bundleIdStr UTF8String];
    }
    
    CFRelease(currentSource);
    return result;
#else
    return "";
#endif
}

bool MacOSSystemIntegration::setCurrentIME(const std::string& bundleId) {
#ifdef __APPLE__
    NSString* targetBundleId = [NSString stringWithUTF8String:bundleId.c_str()];
    
    CFArrayRef inputSources = TISCreateInputSourceList(NULL, false);
    if (!inputSources) {
        return false;
    }
    
    bool success = false;
    CFIndex count = CFArrayGetCount(inputSources);
    
    for (CFIndex i = 0; i < count; i++) {
        TISInputSourceRef inputSource = (TISInputSourceRef)CFArrayGetValueAtIndex(inputSources, i);
        
        CFStringRef sourceId = (CFStringRef)TISGetInputSourceProperty(inputSource, kTISPropertyBundleID);
        if (sourceId && CFStringCompare(sourceId, (__bridge CFStringRef)targetBundleId, 0) == kCFCompareEqualTo) {
            OSStatus status = TISSelectInputSource(inputSource);
            success = (status == noErr);
            break;
        }
    }
    
    CFRelease(inputSources);
    return success;
#else
    return false;
#endif
}

std::vector<std::map<std::string, std::string>> MacOSSystemIntegration::getRunningProcesses() {
    std::vector<std::map<std::string, std::string>> processes;
    
#ifdef __APPLE__
    NSArray* runningApps = [[NSWorkspace sharedWorkspace] runningApplications];
    
    for (NSRunningApplication* app in runningApps) {
        std::map<std::string, std::string> processInfo;
        
        processInfo["pid"] = std::to_string([app processIdentifier]);
        
        if ([app localizedName]) {
            processInfo["name"] = [[app localizedName] UTF8String];
        }
        
        if ([app bundleIdentifier]) {
            processInfo["bundle_id"] = [[app bundleIdentifier] UTF8String];
        }
        
        if ([app bundleURL]) {
            processInfo["path"] = [[[app bundleURL] path] UTF8String];
        }
        
        // Application state
        if ([app isActive]) {
            processInfo["state"] = "active";
        } else if ([app isHidden]) {
            processInfo["state"] = "hidden";
        } else {
            processInfo["state"] = "background";
        }
        
        processes.push_back(processInfo);
    }
#endif
    
    return processes;
}

std::map<std::string, std::string> MacOSSystemIntegration::getCurrentProcessInfo() {
    std::map<std::string, std::string> info;
    
#ifdef __APPLE__
    NSRunningApplication* currentApp = [NSRunningApplication currentApplication];
    
    info["pid"] = std::to_string([currentApp processIdentifier]);
    
    if ([currentApp localizedName]) {
        info["name"] = [[currentApp localizedName] UTF8String];
    }
    
    if ([currentApp bundleIdentifier]) {
        info["bundle_id"] = [[currentApp bundleIdentifier] UTF8String];
    }
    
    if ([currentApp bundleURL]) {
        info["path"] = [[[currentApp bundleURL] path] UTF8String];
    }
    
    // Get executable path
    NSBundle* mainBundle = [NSBundle mainBundle];
    if (mainBundle) {
        NSString* executablePath = [mainBundle executablePath];
        if (executablePath) {
            info["executable_path"] = [executablePath UTF8String];
        }
    }
#endif
    
    return info;
}

bool MacOSSystemIntegration::enableDebugPrivileges() {
    // macOS doesn't have the same debug privilege concept as Windows
    // This is mainly for compatibility with the interface
    return true;
}

std::string MacOSSystemIntegration::getLastError() {
    // Return the last system error
    return strerror(errno);
}

// Bridge functions for Objective-C interoperability
extern "C" {
    MacOSSystemIntegration* createMacOSSystemIntegration() {
        return new MacOSSystemIntegration();
    }
    
    void destroyMacOSSystemIntegration(MacOSSystemIntegration* integration) {
        delete integration;
    }
}