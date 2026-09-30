static EspLayout::Model ReadTeamLayout() {
    auto model=EspLayout::Preset(1);
    model.items[EspLayout::Name].enabled=Config::bAllyName;model.items[EspLayout::Distance].enabled=Config::bAllyDist;
    model.items[EspLayout::Health].enabled=Config::bAllyHp;model.items[EspLayout::Percent].enabled=Config::bAllyPct;
    model.skeleton=Config::bAllySkeleton;model.headDot=false;model.snapline=Config::bAllySnap;model.boxStyle=Config::iAllyBox;
    EspLayout::Normalize(model);return model;
}
