# Branch Predictor Delayed Update

Experiment to determine IP-based branch predictor (BTB) insertion delay.


## Reproduce

The output is stored in [out](./out).

### Ansible

> [!WARNING]
> The ansible playbooks modify the system without requesting confirmation (ex. GRUB config, install packages, ..).

The ansible playbook was only designed to run on x86.
For mobile ARM cores see [Manual](#manual).

```bash
ansible-playbook ../../ansible/run.yaml -e host=<hostname> -e experiment=exp-btb-delay \
    -e experiment_core=<experiment_core> \
    -e experiment_march=<experiment_march>
```


### Manual

#### x86
```bash
export experiment_march=<experiment_march>
export experiment_core=<experiment_core>
export OUT_DIR=./out/manual

# build
make clean
make MARCH=$experiment_march

# save metadata
mkdir -p "$OUT_DIR"
cat > "$OUT_DIR/metadata.json" <<EOF
{
    "hostname": "manual",
    "experiment_core": $experiment_core,
    "experiment_march": "$experiment_march"
}
EOF

# run (sudo required for Linux PFC)
sudo make run CORE=$experiment_core | tee "$OUT_DIR/run.out"
```

#### ARM

We run the experiment on Pixel 6 using a [custom kernel](#compile-custom-kernel).

Android taskset (on Pixel 6) uses bitmasks.
You can calculate `<experiment_core_mask_hex>` as the hex representation of `1 << <experiment_core>`.

```bash
# load custom kernel
adb reboot bootloader
fastboot boot arm/boot.img

# build
export PATH="$PATH:<path to android ndk>/android-ndk-r27c/toolchains/llvm/prebuilt/linux-x86_64/bin/"
make -f Makefile.arm clean
make -f Makefile.arm

# load code on device
adb push main /data/local/tmp
adb shell chmod +x /data/local/tmp/main

# run
experiment_core_mask_hex=<experiment_core_mask_hex>
OUT_DIR="out/pixel6-$experiment_core_mask_hex"
mkdir -p "$OUT_DIR"
adb shell taskset $experiment_core_mask_hex /data/local/tmp/main | tee "$OUT_DIR/run.out"
```


#### Compile custom kernel

We based our custom kernel on the work by [BHI](https://www.usenix.org/conference/usenixsecurity22/presentation/barberis) in their [artifacts](https://github.com/vusec/bhi-spectre-bhb/tree/main/re/arm/bhi_test).
We have ported the patch the a more recent kernel version, only kept the performance counter enable syscall and added the enable bit for arbitrary performance counters.


```bash
# download android stock kernel
mkdir android-kernel && cd android-kernel
repo init -u https://android.googlesource.com/kernel/manifest -b android-gs-raviole-5.10-android14-qpr3
repo sync

# apply the syscall patch
cd aosp
git apply ../../arm/custom_syscall.patch

# compile the custom kernel
cd ..
BUILD_AOSP_KERNEL=1=1 KERNEL_CMDLINE="nokaslr nohlt" ./build_slider.sh
```

The custom kernel will be placed at `out/mixed/dist/boot.img`.


## Analyze

```bash
python3 analyze.py
```

The figure as we presented it in our paper will be placed at [btb_delay.pdf](../../figures/btb_delay.pdf).

### Expected Results

On Intel cores, we observe high numbers of misprediction (>90%) for low number of NOPs (<50) which decrease to almost 0% with higher number of NOPs (>800).
On AMD and ARM cores we do not observe mispredictions (0%) for low numbers of NOPs (<50).
