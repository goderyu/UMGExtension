# 杂记

# 源码理解

## ExpandableArea

该组件的细节面板中，`STYLE`分类下有一个`Style`结构体属性，该属性中的`Rollout Animation Seconds`属性比较有用。该属性设值后，在ExpandableArea被折叠时，会等待该值对应的时间。即这段时间可以触发控件动画，并不会因为播放动画是异步事件而导致效果异常。

查看了源码后，理解了其中的原理。首先，`ExpandableArea`类中有自持的Slate`SExpandableArea`指针，而Slate中，利用`RolloutAnimationSeconds`属性创建了`FCurveSequence`，这个类实例通过`Play/PlayReverse`就可以起到一个阻塞时间的作用。定义的代码在`SExpandableArea::Construct`中：

```c++
RolloutCurve = FCurveSequence(0.0f, InArgs._Style->RolloutAnimationSeconds, ECurveEaseFunction::CubicOut);
```

调用阻塞：

```c++
void SExpandableArea::SetExpanded_Animated(bool bExpanded)
{
	const bool bShouldBeCollapsed = !bExpanded;
	if (bAreaCollapsed != bShouldBeCollapsed)
	{
		bAreaCollapsed = bShouldBeCollapsed;
		if (!bAreaCollapsed)
		{
			RolloutCurve = FCurveSequence(0.0f, RolloutCurve.GetCurve(0).DurationSeconds, ECurveEaseFunction::CubicOut);
			RolloutCurve.Play(this->AsShared());
		}
		else
		{
			RolloutCurve = FCurveSequence(0.0f, RolloutCurve.GetCurve(0).DurationSeconds, ECurveEaseFunction::CubicIn);
			RolloutCurve.PlayReverse(this->AsShared());
		}

		// Allow some section-specific code to be executed when the section becomes visible or collapsed
		OnAreaExpansionChanged.ExecuteIfBound(!bAreaCollapsed);
	}
}
```

可以看到，`OnAreaExpansionChanged`事件分发在`RolloutCurve.Play`之后调用，但是因为`FCurveSequence`有阻塞作用，故这里会阻塞`RolloutCurve.GetCurve(0).DurationSeconds`秒之后再执行`OnAreaExpansionChanged.ExecuteIfBound(!bAreaCollapsed);`