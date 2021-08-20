# 杂记

# 源码理解

## ExpandableArea

该组件的细节面板中，`STYLE`分类下有一个`Style`结构体属性，该属性中的`Rollout Animation Seconds`属性比较有用。该属性设值后，在ExpandableArea被折叠时，会等待该值对应的时间。即这段时间可以触发控件动画，并不会因为播放动画是异步事件而导致效果异常。

查看了源码后，理解了其中的原理。首先，`ExpandableArea`类中有自持的Slate`SExpandableArea`指针，而Slate中，利用`RolloutAnimationSeconds`属性创建了`FCurveSequence`，这个类实例就可以起到一个阻塞时间的作用。代码在`SExpandableArea::Construct`中，调用了：

```c++
RolloutCurve = FCurveSequence(0.0f, InArgs._Style->RolloutAnimationSeconds, ECurveEaseFunction::CubicOut);
```

