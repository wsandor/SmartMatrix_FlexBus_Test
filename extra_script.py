Import("env")

def after_upload(source, target, env):
    print("Delay 2s while uploading...")
    import time
    time.sleep(2)
    print("Done!")

env.AddPostAction("upload", after_upload)