# BlackHat USA 2025 Demo

Small demonstration of the BPI exploit written for BlackHat.
It is based on _exp-end2end_ and mostly just integrates the components and improves the speed a bit.

## Reproduce

>[!TIP]
> The attack was specifically designed for Intel Raptor Lake processors, your mileage on other processors **will** vary.
> However, thanks to our artifact evaluators, we know that the attack also works on Xeon silver 4514Y (and subsequently our Xeon Silver 4510).
> Since the attack implementation expects 48-bit address spaces (although this is not in general a hard requirement), we need to add `no5lvl` to the kernel commandline on these processors.

The attack requires you to use the specific kernel binary for which it was developed. For your convenience, we have included the Ubuntu 24.04 kernel packages that we used in [kernel](./kernel-v6.8.0-47-generic).

Once you have the right kernel installed, you can just run the end-to-end attack.
```bash
make run
```
