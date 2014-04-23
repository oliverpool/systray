package main

import (
    "flag"
    "github.com/AnimationMentor/systray"
    "github.com/golang/glog"
    "os"
    "path"
    "path/filepath"
    "runtime"
)

func main() {
    //// Let us tap into the glog flags (like -alsologtostderr=true -log_dir=".")
    //
    
    var iconDirFlag = flag.String("icon_path", ".", "Path to icons files")
    var iconNameFlag = flag.String("icon_name", "sysapp.ico", "Name of icon to show")
    flag.Parse()

    var iconDir string
    if filepath.IsAbs(*iconDirFlag) {
        iconDir = *iconDirFlag
    } else {
        // Default our icon path to be relative to the exe
        thisExe := os.Args[0]
        thisDir := path.Dir(thisExe)
        iconDir = path.Join(thisDir, *iconDirFlag)
    }

    tray := systray.New(iconDir, ".")

    //// Set some test menu items
    items := make([]systray.CallbackInfo, 2)
    items[0] = systray.CallbackInfo {
        ItemName : "Test Menu 1",
        Callback : func() {
            println("Got menu 1")
        },
    }
    items[1] = systray.CallbackInfo {
        ItemName : "Test Menu 2",
        Callback : func() {
            println("Got menu 2")
        },
    }
    tray.AddSystrayMenuItems(items)

    err := tray.Show(*iconNameFlag, "Sysapp Test")
    if err != nil {
        glog.Infoln(err.Error())
    }

    runtime.LockOSThread()
    tray.Run()
    runtime.UnlockOSThread()
}
