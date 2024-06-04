# Daily theater button for GL client
This document details the steps to recreate the daily theater feature for SIFAS GL.

This use the 64 bits version of the games. The 32 bits version, if possible, would follow the same process.
## Original source code for JP

```C
HomeDM_DailyTheaterButtonVO * HomeDM_GetDailyTheaterButtonVO(MethodInfo *method) {
    undefined auVar1 [16];
    if ((DAT_075989ec & 1) == 0) {
        FUN_01f335c0(0x8585);
        DAT_075989ec = 1;
    }
    auVar1 = thunk_FUN_01f6d8c8(HomeDM_DailyTheaterButtonVO__TypeInfo);
    HomeDM_DailyTheaterButtonVO__ctor_1(auVar1._0_8_,auVar1._8_8_);
    return auVar1._0_8_;
}
```

## FUN_01f335c0
### JP source code
This function seems to be some sort of initial check, possibly coded something like this:

```C
HomeDM_DailyTheaterButtonVO * HomeDM_GetDailyTheaterButtonVO(MethodInfo *method) {
    static bool done_init = false;
    if (!done_init) {
        FUN_01f335c0(0x8585);
        done_init = true;
    }
    ...
}
```

`FUN_01f335c0` is used pretty much everywhere, so this is likely related to il2cpp. A few examples of it being used:

```C
    if ((DAT_07598a02 & 1) == 0) {
        FUN_01f335c0(0x3eeb); // 0x7594B17 diff between data and param
        DAT_07598a02 = 1;
    }

    if ((DAT_075962ca & 1) == 0) {
        FUN_01f335c0(0xe2e4); // 0x7587FE6 diff between data and param
        DAT_075962ca = 1; 
        method = extraout_x1;
    }

    if ((DAT_07596af5 & 1) == 0) {
        FUN_01f335c0(0x38d0);
        DAT_07596af5 = 1;
        method = extraout_x1;
    }

    if ((DAT_075a53c7 & 1) == 0) {
        FUN_01f335c0(0x8014);
        DAT_075a53c7 = 1;
        method = extraout_x1;
    }
```
The params to that function seems to be a counting values:

```C
HomeDM_ChallengeButtonVO * HomeDM_GetChallengeBeginnerButtonVO(MethodInfo *method)
{
    ...
    if ((DAT_075989eb & 1) == 0) {
        FUN_01f335c0(0x8584);
        DAT_075989eb = 1;
    }
    ...
}

HomeDM_DailyTheaterButtonVO * HomeDM_GetDailyTheaterButtonVO(MethodInfo *method) 
{
    ...
    if ((DAT_075989ec & 1) == 0) {
        FUN_01f335c0(0x8585);
        DAT_075989ec = 1;
    }
    ...
}
```


### GL reconstruction

We have the related function in GL:
```C
HomeDM_ChallengeButtonVO * HomeDM_GetChallengeBeginnerButtonVO(MethodInfo *method)
{
    if ((DAT_075a86b3 & 1) == 0) {
        FUN_01f4149c(0x8588);
        DAT_075a86b3 = 1;
    }
}
```

FUN_01f4149c (gl) should be the equivalent of FUN_01f335c0 (jp).

We can speculate that we should call FUN_01f4149c with 0x8589 for HomeDM_DailyTheaterButtonVO, and we can speculate that we should use DAT_075a86b4. However:

- There doesn't seems to be a function that call FUN_01f4149c with 0x8589
- but DAT_075a86b4 is already used in HomeDM_GetSubscriptionPassButtonVO
- We will use the following trick to reuse DAT_075a86b3:

  - Because we compiler optimised bool checking into a &1, we actually can use the other bits of that data
  - So we will check with &2 instead

So the reconstruction on GL should read:

```C

HomeDM_DailyTheaterButtonVO * HomeDM_GetDailyTheaterButtonVO(MethodInfo *method)
{
    if ((DAT_075a86b3 & 2) == 0) {
        FUN_01f4149c(0x8589);
        DAT_075a86b3 = 3; // this assume we will call HomeDM_GetChallengeBeginnerButtonVO at least once before HomeDM_GetDailyTheaterButtonVO
    }
}
```


## thunk_FUN_01f6d8c8

### JP source code

This is most likely an allocator

thunk_FUN_01f6d8c8 if often called with ...__TypeInfo and return a pointer.

Presumablly, the allocation would do memory management:

```C
HomeDM_DailyTheaterButtonVO * HomeDM_GetDailyTheaterButtonVO(MethodInfo *method) {
    auVar1 = thunk_FUN_01f6d8c8(HomeDM_DailyTheaterButtonVO__TypeInfo);
}
```

Some other usage:

```C
HomeDM_SubscriptionPassButtonVO * HomeDM_GetSubscriptionPassButtonVO(MethodInfo *method)
{
    ...
    auVar1 = thunk_FUN_01f6d8c8(HomeDM_SubscriptionPassButtonVO__TypeInfo);
    HomeDM_SubscriptionPassButtonVO__ctor(auVar1._0_8_,auVar1._8_8_);
    return auVar1._0_8_;
}
```

### GL reconstruction
We have similar usage in GL: 
```C
HomeDM_SubscriptionPassButtonVO * HomeDM_GetSubscriptionPassButtonVO(MethodInfo *method)
{
    auVar1 = thunk_FUN_01f7b7a4(HomeDM_SubscriptionPassButtonVO__TypeInfo);
    HomeDM_SubscriptionPassButtonVO__ctor(auVar1._0_8_,auVar1._8_8_);
    return auVar1._0_8_;
}
```

so thunk_FUN_01f7b7a4 (gl) should be the same as thunk_FUN_01f6d8c8(jp), and we want to call thunk_FUN_01f7b7a4 on HomeDM_DailyTheaterButtonVO__TypeInfo. Sadly this label doesn't exist in GL:

- It could be that the label exist somewhere else, but ghidra / il2cpp inspector failed to generate it.
- However, it's likely that the label isn't there.
- For JP we have the data region of these as follow:

```asm
                             HomeDM_ChallengeButtonVO__TypeInfo              XREF[2]:     02540bc8 (R) , 07098b90 (*)   
        074f5a60                 HomeDM_C   NaP                                              ChallengeButtonVO
                             HomeDM_DailyTheaterButtonVO__TypeInfo           XREF[2]:     02540cc8 (R) , 0701b0c8 (*)   
        074f5a68                 HomeDM_D   NaP                                              DailyTheaterButtonVO
                             HomeDM_SubscriptionPassButtonVO__TypeInfo       XREF[2]:     02540ea8 (R) , 06fd59e8 (*)   
        074f5a70                 HomeDM_S   NaP                                              SubscriptionPassButtonVO
```
- However for GL, HomeDM_DailyTheaterButtonVO__TypeInfo is missing and there's no data gap:

```asm
                             HomeDM_ChallengeButtonVO__TypeInfo              XREF[2]:     0258e254 (R) , 070a66b8 (*)   
        07503d58                 HomeDM_C   NaP                                              ChallengeButtonVO
                             HomeDM_SubscriptionPassButtonVO__TypeInfo       XREF[2]:     HomeDM_GetSubscriptionPassButton
                                                                                          06fe2760 (*)   
        07503d60                 HomeDM_S   NaP                                              SubscriptionPassButtonVO
```

If we assume that this is a simple allocator, then we can allocate for a similar type (or just malloc directly) and it should works. HomeDM_ChallengeButtonVO has the same size and alignment as HomeDM_DailyTheaterButtonVO, so we can use it.

So the reconstruction on GL should read:

```C

HomeDM_DailyTheaterButtonVO * HomeDM_GetDailyTheaterButtonVO(MethodInfo *method)
{
    auVar1 = thunk_FUN_01f7b7a4(HomeDM_ChallengeButtonVO__TypeInfo);
}
```

## Constructor and return
The constructor is the same for GL and JP, and assuming it can works, the whole GL reconstruction should read:

```C

HomeDM_DailyTheaterButtonVO* HomeDM_GetDailyTheaterButtonVO(MethodInfo *method)
{
    undefined auVar1 [16];
    if ((DAT_075a86b3 & 2) == 0) {
        FUN_01f4149c(0x8589);
        DAT_075a86b3 |= 2;
    }
    auVar1 = thunk_FUN_01f7b7a4(HomeDM_ChallengeButtonVO__TypeInfo);
    HomeDM_DailyTheaterButtonVO__ctor_1(auVar1._0_8_,auVar1._8_8_);
    return auVar1._0_8_;
}
```

Furthermore, we also need to patch the game to call to the new function. This is easiest done with a `b` instruction in the original function.

### Assembly plan
The original JP function span from 0x02540c90 to 0x02540ce7, so 0x58 bytes or 0x16 = 22 instructions.

For the GL client, we can use the space left behind by patching TitleDM_IsHoldingTestModePeriod to reconstruct this:

- TitleDM_IsHoldingTestModePeriod requires 2 instruction to return true.
- So we are left with the space from 0x023eafac to 0x023eb017 before we run into another function.
- So we have 0x6c bytes or 0x1b = 27 instructions, hopefully this is still enough for all the stuff we need to do.
- If necessary, since the last instruction of original TitleDM_IsHoldingTestModePeriod is branch without return (b), we can potentially use where it branch to too.
- Finally, we also need to branch from the actual HomeDM_GetDailyTheaterButtonVO to the newly constructed function, but this is trivial.

### Assembly result
Calling FUN_01f4149c(0x8589) crashs, so some sort of checking is going on there.

We can call FUN_01f4149c(0x8588) to be the same as the ChallengeButtonVO, and it just works.

So the function actually reads:

```C

HomeDM_DailyTheaterButtonVO* HomeDM_GetDailyTheaterButtonVO(MethodInfo *method)
{
    undefined auVar1 [16];
    if ((DAT_075a86b3 & 2) == 0) {
        FUN_01f4149c(0x8588);
        DAT_075a86b3 |= 2;
    }
    auVar1 = thunk_FUN_01f7b7a4(HomeDM_ChallengeButtonVO__TypeInfo);
    HomeDM_DailyTheaterButtonVO__ctor_1(auVar1._0_8_,auVar1._8_8_);
    return auVar1._0_8_;
}
```

See the .shed file for detailed assembly