import svgPaths from "./svg-z3gpm8882p";
import imgUserProfileAvatar from "../../assets/user.png";

function Container() {
  return (
    <div className="content-stretch flex flex-col items-start relative shrink-0" data-name="Container">
      <div className="flex flex-col font-['Inter:Black',sans-serif] font-black h-[28px] justify-center leading-[0] not-italic relative shrink-0 text-[#00a3ff] text-[18px] tracking-[1.8px] w-[204.27px]">
        <p className="leading-[28px]">{`NET_VISOR // CORE`}</p>
      </div>
    </div>
  );
}

function Container1() {
  return (
    <div className="flex-[1_0_0] min-w-px relative" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-start overflow-clip relative rounded-[inherit] size-full">
        <div className="flex flex-col font-['Space_Grotesk:Regular',sans-serif] font-normal justify-center leading-[0] relative shrink-0 text-[#3f4852] text-[13px] w-full">
          <p className="leading-[normal]">SEARCH_QUERY...</p>
        </div>
      </div>
    </div>
  );
}

function Input() {
  return (
    <div className="bg-[#171f33] h-[32px] relative rounded-[4px] shrink-0 w-[256px]" data-name="Input">
      <div className="content-stretch flex items-start justify-center overflow-clip pl-[33px] pr-[17px] py-[7.5px] relative rounded-[inherit] size-full">
        <Container1 />
      </div>
      <div aria-hidden="true" className="absolute border border-[#31394d] border-solid inset-0 pointer-events-none rounded-[4px]" />
    </div>
  );
}

function Container2() {
  return (
    <div className="absolute bottom-[21.88%] content-stretch flex flex-col items-start left-[8px] top-[21.88%]" data-name="Container">
      <div className="relative shrink-0 size-[13.5px]" data-name="Icon">
        <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 13.5 13.5">
          <path d={svgPaths.p2500af80} fill="var(--fill-0, #3F4852)" id="Icon" />
        </svg>
      </div>
    </div>
  );
}

function SearchBarOnLeft() {
  return (
    <div className="content-stretch flex flex-col items-start relative shrink-0" data-name="Search Bar (on_left)">
      <Input />
      <Container2 />
    </div>
  );
}

function SearchBarOnLeftMargin() {
  return (
    <div className="content-stretch flex flex-col items-start pl-[24px] relative shrink-0" data-name="Search Bar (on_left):margin">
      <SearchBarOnLeft />
    </div>
  );
}

function BrandSearchContainer() {
  return (
    <div className="h-full relative shrink-0" data-name="Brand & Search Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex items-center relative size-full">
        <Container />
        <SearchBarOnLeftMargin />
      </div>
    </div>
  );
}

function Link() {
  return (
    <div className="h-full relative shrink-0" data-name="Link">
      <div aria-hidden="true" className="absolute border-[#00a3ff] border-b-2 border-solid inset-0 pointer-events-none" />
      <div className="flex flex-row items-center size-full">
        <div className="content-stretch flex items-center pb-[21px] pt-[18px] px-[16px] relative size-full">
          <div className="flex flex-col font-['Inter:Bold',sans-serif] font-bold h-[16px] justify-center leading-[0] not-italic relative shrink-0 text-[#00a3ff] text-[12px] tracking-[-0.6px] uppercase w-[53.5px]">
            <p className="leading-[16px]">MONITOR</p>
          </div>
        </div>
      </div>
    </div>
  );
}

function Link1() {
  return (
    <div className="flex-[1_0_0] min-h-px relative" data-name="Link">
      <div className="flex flex-row items-center size-full">
        <div className="content-stretch flex items-center pb-[20px] pt-[19px] px-[16px] relative size-full">
          <div className="flex flex-col font-['Inter:Bold',sans-serif] font-bold h-[16px] justify-center leading-[0] not-italic relative shrink-0 text-[#64748b] text-[12px] tracking-[-0.6px] uppercase w-[46.98px]">
            <p className="leading-[16px]">TRAFFIC</p>
          </div>
        </div>
      </div>
    </div>
  );
}

function LinkMargin() {
  return (
    <div className="content-stretch flex flex-col h-full items-start justify-center pl-[4px] relative shrink-0" data-name="Link:margin">
      <Link1 />
    </div>
  );
}

function Link2() {
  return (
    <div className="flex-[1_0_0] min-h-px relative" data-name="Link">
      <div className="flex flex-row items-center size-full">
        <div className="content-stretch flex items-center pb-[20px] pt-[19px] px-[16px] relative size-full">
          <div className="flex flex-col font-['Inter:Bold',sans-serif] font-bold h-[16px] justify-center leading-[0] not-italic relative shrink-0 text-[#64748b] text-[12px] tracking-[-0.6px] uppercase w-[56.16px]">
            <p className="leading-[16px]">SECURITY</p>
          </div>
        </div>
      </div>
    </div>
  );
}

function LinkMargin1() {
  return (
    <div className="content-stretch flex flex-col h-full items-start justify-center pl-[4px] relative shrink-0" data-name="Link:margin">
      <Link2 />
    </div>
  );
}

function Link3() {
  return (
    <div className="flex-[1_0_0] min-h-px relative" data-name="Link">
      <div className="flex flex-row items-center size-full">
        <div className="content-stretch flex items-center pb-[20px] pt-[19px] px-[16px] relative size-full">
          <div className="flex flex-col font-['Inter:Bold',sans-serif] font-bold h-[16px] justify-center leading-[0] not-italic relative shrink-0 text-[#64748b] text-[12px] tracking-[-0.6px] uppercase w-[43.98px]">
            <p className="leading-[16px]">ASSETS</p>
          </div>
        </div>
      </div>
    </div>
  );
}

function LinkMargin2() {
  return (
    <div className="content-stretch flex flex-col h-full items-start justify-center pl-[4px] relative shrink-0" data-name="Link:margin">
      <Link3 />
    </div>
  );
}

function NavigationLinksCenteredArea() {
  return (
    <div className="h-full relative shrink-0" data-name="Navigation Links (Centered area)">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex items-center relative size-full">
        <Link />
        <LinkMargin />
        <LinkMargin1 />
        <LinkMargin2 />
      </div>
    </div>
  );
}

function Button() {
  return (
    <div className="bg-[#00a3ff] content-stretch flex flex-col items-center justify-center px-[17px] py-[7px] relative rounded-[4px] shrink-0" data-name="Button">
      <div aria-hidden="true" className="absolute border border-[#00a3ff] border-solid inset-0 pointer-events-none rounded-[4px]" />
      <div className="flex flex-col font-['Space_Grotesk:Bold',sans-serif] font-bold h-[16px] justify-center leading-[0] relative shrink-0 text-[#0b1326] text-[11px] text-center tracking-[0.88px] w-[83.36px]">
        <p className="leading-[16px]">DEPLOY_VLAN</p>
      </div>
    </div>
  );
}

function Container3() {
  return (
    <div className="h-[13.333px] relative shrink-0 w-[16.667px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 16.6667 13.3333">
        <g id="Container">
          <path d={svgPaths.p3d291c80} fill="var(--fill-0, #88919D)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Button1() {
  return (
    <div className="-translate-y-1/2 absolute content-stretch flex items-center justify-center left-[17px] rounded-[4px] size-[32px] top-1/2" data-name="Button">
      <Container3 />
    </div>
  );
}

function Container4() {
  return (
    <div className="h-[16.667px] relative shrink-0 w-[16.75px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 16.75 16.6667">
        <g id="Container">
          <path d={svgPaths.p18e22d80} fill="var(--fill-0, #88919D)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Button2() {
  return (
    <div className="content-stretch flex items-center justify-center relative rounded-[4px] shrink-0 size-[32px]" data-name="Button">
      <Container4 />
    </div>
  );
}

function ButtonMargin() {
  return (
    <div className="absolute content-stretch flex flex-col h-[32px] items-start left-[89px] pl-[8px] top-0 w-[40px]" data-name="Button:margin">
      <Button2 />
    </div>
  );
}

function Container5() {
  return (
    <div className="h-[16.667px] relative shrink-0 w-[13.333px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 13.3333 16.6667">
        <g id="Container">
          <path d={svgPaths.p2ab08e80} fill="var(--fill-0, #88919D)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Button3() {
  return (
    <div className="content-stretch flex items-center justify-center relative rounded-[4px] shrink-0 size-[32px]" data-name="Button">
      <Container5 />
      <div className="absolute bg-[#ffb4ab] right-[6px] rounded-[9999px] size-[8px] top-[6px]" data-name="Background" />
    </div>
  );
}

function ButtonMargin1() {
  return (
    <div className="absolute content-stretch flex flex-col h-[32px] items-start left-[49px] pl-[8px] top-0 w-[40px]" data-name="Button:margin">
      <Button3 />
    </div>
  );
}

function VerticalBorder() {
  return (
    <div className="h-[32px] relative shrink-0 w-[129px]" data-name="VerticalBorder">
      <div aria-hidden="true" className="absolute border-[#31394d] border-l border-solid inset-0 pointer-events-none" />
      <Button1 />
      <ButtonMargin />
      <ButtonMargin1 />
    </div>
  );
}

function Margin() {
  return (
    <div className="content-stretch flex flex-col h-[32px] items-start pl-[16px] relative shrink-0" data-name="Margin">
      <VerticalBorder />
    </div>
  );
}

function TrailingActions() {
  return (
    <div className="h-full relative shrink-0" data-name="Trailing Actions">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex items-center relative size-full">
        <Button />
        <Margin />
      </div>
    </div>
  );
}

function TopNavBarSharedComponent() {
  return (
    <div className="bg-[#060e20] h-[56px] relative shrink-0 w-full z-[2]" data-name="TopNavBar (Shared Component)">
      <div aria-hidden="true" className="absolute border-[#31394d] border-b border-solid inset-0 pointer-events-none" />
      <div className="flex flex-row items-center size-full">
        <div className="content-stretch flex items-center justify-between pb-px px-[16px] relative size-full">
          <BrandSearchContainer />
          <NavigationLinksCenteredArea />
          <TrailingActions />
        </div>
      </div>
    </div>
  );
}

function UserProfileAvatar() {
  return (
    <div className="flex-[1_0_0] h-full min-w-px mix-blend-luminosity opacity-80 relative" data-name="User Profile Avatar">
      <div className="absolute bg-clip-padding border-0 border-[transparent] border-solid inset-0 overflow-hidden pointer-events-none">
        <img alt="" className="absolute left-0 max-w-none size-full top-0" src={imgUserProfileAvatar} />
      </div>
    </div>
  );
}

function BackgroundBorder() {
  return (
    <div className="bg-[#222a3d] relative rounded-[4px] shrink-0 size-[40px]" data-name="Background+Border">
      <div className="content-stretch flex items-center justify-center overflow-clip p-px relative rounded-[inherit] size-full">
        <UserProfileAvatar />
      </div>
      <div aria-hidden="true" className="absolute border border-[#3f4852] border-solid inset-0 pointer-events-none rounded-[4px]" />
    </div>
  );
}

function Margin1() {
  return (
    <div className="h-[48px] relative shrink-0 w-[40px]" data-name="Margin">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-start pb-[8px] relative size-full">
        <BackgroundBorder />
      </div>
    </div>
  );
}

function Container6() {
  return (
    <div className="h-[13.75px] relative shrink-0 w-full" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid overflow-clip relative rounded-[inherit] size-full">
        <div className="-translate-x-1/2 -translate-y-1/2 absolute flex flex-col font-['Space_Grotesk:Bold',sans-serif] font-bold h-[14px] justify-center leading-[0] left-[calc(50%-0.62px)] overflow-hidden text-[#98cbff] text-[11px] text-center text-ellipsis top-[6px] tracking-[0.88px] w-[69.77px] whitespace-nowrap">
          <p className="leading-[13.75px] overflow-hidden text-ellipsis">OPERATOR_01</p>
        </div>
      </div>
    </div>
  );
}

function Container7() {
  return (
    <div className="relative shrink-0 w-full" data-name="Container">
      <div className="flex flex-col items-center overflow-clip rounded-[inherit] size-full">
        <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-center pl-[4px] pr-[6.69px] relative size-full">
          <div className="flex flex-col font-['Space_Grotesk:Regular',sans-serif] font-normal h-[18px] justify-center leading-[0] overflow-hidden relative shrink-0 text-[#88919d] text-[13px] text-center text-ellipsis w-[68.31px] whitespace-nowrap">
            <p className="leading-[18px] overflow-hidden text-ellipsis">SYS_ADMIN_NODE_A</p>
          </div>
        </div>
      </div>
    </div>
  );
}

function HeaderOperatorInfo() {
  return (
    <div className="content-stretch flex flex-col items-center pb-[17px] pt-[16px] relative shrink-0 w-full" data-name="Header (Operator Info)">
      <div aria-hidden="true" className="absolute border-[rgba(49,57,77,0.5)] border-b border-solid inset-0 pointer-events-none" />
      <Margin1 />
      <Container6 />
      <Container7 />
    </div>
  );
}

function HeaderOperatorInfoMargin() {
  return (
    <div className="relative shrink-0 w-full" data-name="Header (Operator Info):margin">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-start pb-[8px] relative size-full">
        <HeaderOperatorInfo />
      </div>
    </div>
  );
}

function Margin2() {
  return (
    <div className="h-[22px] relative shrink-0 w-[18px]" data-name="Margin">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 18 22">
        <g id="Margin">
          <path d={svgPaths.p20793584} fill="var(--fill-0, #00A3FF)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Container8() {
  return (
    <div className="relative shrink-0 w-full" data-name="Container">
      <div className="flex flex-col items-center overflow-clip rounded-[inherit] size-full">
        <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-center px-[4px] relative size-full">
          <div className="flex flex-col font-['Inter:Medium',sans-serif] font-medium h-[22px] justify-center leading-[0] not-italic relative shrink-0 text-[#00a3ff] text-[10px] text-center tracking-[-0.25px] w-[60.2px]">
            <p className="leading-[22px]">DASHBOARD</p>
          </div>
        </div>
      </div>
    </div>
  );
}

function Link4() {
  return (
    <div className="bg-[rgba(0,163,255,0.1)] content-stretch flex flex-col items-center justify-center pr-[2px] py-[16px] relative shrink-0 w-full" data-name="Link">
      <div aria-hidden="true" className="absolute border-[#00a3ff] border-r-2 border-solid inset-0 pointer-events-none" />
      <Margin2 />
      <Container8 />
    </div>
  );
}

function Margin3() {
  return (
    <div className="h-[27px] relative shrink-0 w-[24px]" data-name="Margin">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 24 27">
        <g id="Margin">
          <path d={svgPaths.p80d2080} fill="var(--fill-0, #3F4852)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Container9() {
  return (
    <div className="relative shrink-0 w-full" data-name="Container">
      <div className="flex flex-col items-center overflow-clip rounded-[inherit] size-full">
        <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-center px-[4px] relative size-full">
          <div className="flex flex-col font-['Inter:Medium',sans-serif] font-medium h-[22px] justify-center leading-[0] not-italic relative shrink-0 text-[#3f4852] text-[10px] text-center tracking-[-0.25px] w-[53.2px]">
            <p className="leading-[22px]">TOPOLOGY</p>
          </div>
        </div>
      </div>
    </div>
  );
}

function Link5() {
  return (
    <div className="content-stretch flex flex-col items-center justify-center pr-[2px] py-[16px] relative shrink-0 w-full" data-name="Link">
      <div aria-hidden="true" className="absolute border-[rgba(0,0,0,0)] border-r-2 border-solid inset-0 pointer-events-none" />
      <Margin3 />
      <Container9 />
    </div>
  );
}

function Margin4() {
  return (
    <div className="h-[17px] relative shrink-0 w-[20px]" data-name="Margin">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 20 17">
        <g id="Margin">
          <path d={svgPaths.pa9e6b00} fill="var(--fill-0, #3F4852)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Container10() {
  return (
    <div className="relative shrink-0 w-full" data-name="Container">
      <div className="flex flex-col items-center overflow-clip rounded-[inherit] size-full">
        <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-center px-[4px] relative size-full">
          <div className="flex flex-col font-['Inter:Medium',sans-serif] font-medium h-[22px] justify-center leading-[0] not-italic relative shrink-0 text-[#3f4852] text-[10px] text-center tracking-[-0.25px] w-[42.94px]">
            <p className="leading-[22px]">METRICS</p>
          </div>
        </div>
      </div>
    </div>
  );
}

function Link6() {
  return (
    <div className="content-stretch flex flex-col items-center justify-center pr-[2px] py-[16px] relative shrink-0 w-full" data-name="Link">
      <div aria-hidden="true" className="absolute border-[rgba(0,0,0,0)] border-r-2 border-solid inset-0 pointer-events-none" />
      <Margin4 />
      <Container10 />
    </div>
  );
}

function Margin5() {
  return (
    <div className="h-[24px] relative shrink-0 w-[16px]" data-name="Margin">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 16 24">
        <g id="Margin">
          <path d={svgPaths.p2bdb86e0} fill="var(--fill-0, #3F4852)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Container11() {
  return (
    <div className="relative shrink-0 w-full" data-name="Container">
      <div className="flex flex-col items-center overflow-clip rounded-[inherit] size-full">
        <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-center pl-[4px] pr-[7.64px] relative size-full">
          <div className="flex flex-col font-['Inter:Medium',sans-serif] font-medium h-[22px] justify-center leading-[0] not-italic overflow-hidden relative shrink-0 text-[#3f4852] text-[10px] text-center text-ellipsis tracking-[-0.25px] w-[65.36px] whitespace-nowrap">
            <p className="leading-[22px] overflow-hidden text-ellipsis">THREAT_HUNT</p>
          </div>
        </div>
      </div>
    </div>
  );
}

function Link7() {
  return (
    <div className="content-stretch flex flex-col items-center justify-center pr-[2px] py-[16px] relative shrink-0 w-full" data-name="Link">
      <div aria-hidden="true" className="absolute border-[rgba(0,0,0,0)] border-r-2 border-solid inset-0 pointer-events-none" />
      <Margin5 />
      <Container11 />
    </div>
  );
}

function Margin6() {
  return (
    <div className="h-[22px] relative shrink-0 w-[18px]" data-name="Margin">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 18 22">
        <g id="Margin">
          <path d={svgPaths.p254c2600} fill="var(--fill-0, #3F4852)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Container12() {
  return (
    <div className="relative shrink-0 w-full" data-name="Container">
      <div className="flex flex-col items-center overflow-clip rounded-[inherit] size-full">
        <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-center px-[4px] relative size-full">
          <div className="flex flex-col font-['Inter:Medium',sans-serif] font-medium h-[22px] justify-center leading-[0] not-italic relative shrink-0 text-[#3f4852] text-[10px] text-center tracking-[-0.25px] w-[42.45px]">
            <p className="leading-[22px]">ARCHIVE</p>
          </div>
        </div>
      </div>
    </div>
  );
}

function Link8() {
  return (
    <div className="content-stretch flex flex-col items-center justify-center pr-[2px] py-[16px] relative shrink-0 w-full" data-name="Link">
      <div aria-hidden="true" className="absolute border-[rgba(0,0,0,0)] border-r-2 border-solid inset-0 pointer-events-none" />
      <Margin6 />
      <Container12 />
    </div>
  );
}

function Tabs() {
  return (
    <div className="flex-[1_0_0] min-h-px relative w-full" data-name="Tabs">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-start overflow-clip relative rounded-[inherit] size-full">
        <Link4 />
        <Link5 />
        <Link6 />
        <Link7 />
        <Link8 />
      </div>
    </div>
  );
}

function Margin7() {
  return (
    <div className="h-[19px] relative shrink-0 w-[15px]" data-name="Margin">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 15 19">
        <g id="Margin">
          <path d={svgPaths.p1ed0bdc0} fill="var(--fill-0, #3F4852)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Container13() {
  return (
    <div className="relative shrink-0 w-full" data-name="Container">
      <div className="flex flex-col items-center overflow-clip rounded-[inherit] size-full">
        <div className="content-stretch flex flex-col items-center px-[4px] relative size-full">
          <div className="flex flex-col font-['Inter:Medium',sans-serif] font-medium h-[22px] justify-center leading-[0] not-italic relative shrink-0 text-[#3f4852] text-[9px] text-center tracking-[-0.225px] w-[63.69px]">
            <p className="leading-[22px]">SYSTEM_LOGS</p>
          </div>
        </div>
      </div>
    </div>
  );
}

function Link9() {
  return (
    <div className="relative shrink-0 w-full" data-name="Link">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-center justify-center py-[12px] relative size-full">
        <Margin7 />
        <Container13 />
      </div>
    </div>
  );
}

function Margin8() {
  return (
    <div className="h-[19px] relative shrink-0 w-[15px]" data-name="Margin">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 15 19">
        <g id="Margin">
          <path d={svgPaths.p2b55a3c0} fill="var(--fill-0, #3F4852)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Container14() {
  return (
    <div className="relative shrink-0 w-full" data-name="Container">
      <div className="flex flex-col items-center overflow-clip rounded-[inherit] size-full">
        <div className="content-stretch flex flex-col items-center px-[4px] relative size-full">
          <div className="flex flex-col font-['Inter:Medium',sans-serif] font-medium h-[22px] justify-center leading-[0] not-italic relative shrink-0 text-[#3f4852] text-[9px] text-center tracking-[-0.225px] w-[36.56px]">
            <p className="leading-[22px]">LOGOUT</p>
          </div>
        </div>
      </div>
    </div>
  );
}

function Link10() {
  return (
    <div className="relative shrink-0 w-full" data-name="Link">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-center justify-center py-[12px] relative size-full">
        <Margin8 />
        <Container14 />
      </div>
    </div>
  );
}

function FooterTabs() {
  return (
    <div className="relative shrink-0 w-full" data-name="Footer Tabs">
      <div aria-hidden="true" className="absolute border-[rgba(49,57,77,0.5)] border-solid border-t inset-0 pointer-events-none" />
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-start pb-[8px] pt-[9px] relative size-full">
        <Link9 />
        <Link10 />
      </div>
    </div>
  );
}

function SideNavBarSharedComponent() {
  return (
    <div className="bg-[#0f172a] content-stretch flex flex-col h-full items-start justify-between pr-px relative shrink-0 w-[80px] z-[2]" data-name="SideNavBar (Shared Component)">
      <div aria-hidden="true" className="absolute border-[#31394d] border-r border-solid inset-0 pointer-events-none" />
      <HeaderOperatorInfoMargin />
      <Tabs />
      <FooterTabs />
    </div>
  );
}

function Container16() {
  return (
    <div className="h-[15.333px] relative shrink-0 w-[16px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 16 15.3333">
        <g id="Container">
          <path d={svgPaths.pf2d0700} fill="var(--fill-0, #00A3FF)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Heading2Margin() {
  return (
    <div className="content-stretch flex flex-col items-start pl-[8px] relative shrink-0" data-name="Heading 2:margin">
      <div className="flex flex-col font-['Space_Grotesk:Bold',sans-serif] font-bold h-[16px] justify-center leading-[0] relative shrink-0 text-[#dae2fd] text-[11px] tracking-[0.88px] w-[160.17px]">
        <p className="leading-[16px]">NETWORK_TOPOLOGY_MAP</p>
      </div>
    </div>
  );
}

function Border() {
  return (
    <div className="content-stretch flex flex-col items-start px-[9px] py-[3px] relative rounded-[4px] shrink-0" data-name="Border">
      <div aria-hidden="true" className="absolute border border-[rgba(0,163,255,0.5)] border-solid inset-0 pointer-events-none rounded-[4px]" />
      <div className="flex flex-col font-['Space_Grotesk:Regular',sans-serif] font-normal h-[22px] justify-center leading-[0] relative shrink-0 text-[#00a3ff] text-[9px] w-[17.75px]">
        <p className="leading-[22px]">LIVE</p>
      </div>
    </div>
  );
}

function Margin9() {
  return (
    <div className="content-stretch flex flex-col items-start pl-[8px] relative shrink-0" data-name="Margin">
      <Border />
    </div>
  );
}

function Container15() {
  return (
    <div className="relative shrink-0" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex items-center relative size-full">
        <Container16 />
        <Heading2Margin />
        <Margin9 />
      </div>
    </div>
  );
}

function Container18() {
  return (
    <div className="relative shrink-0 size-[12px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 12 12">
        <g id="Container">
          <path d={svgPaths.p316a0080} fill="var(--fill-0, #88919D)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Button4() {
  return (
    <div className="content-stretch flex items-center justify-center relative rounded-[4px] shrink-0 size-[24px]" data-name="Button">
      <Container18 />
    </div>
  );
}

function Container19() {
  return (
    <div className="relative shrink-0 size-[12px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 12 12">
        <g id="Container">
          <path d={svgPaths.p4be2240} fill="var(--fill-0, #88919D)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Button5() {
  return (
    <div className="content-stretch flex items-center justify-center relative rounded-[4px] shrink-0 size-[24px]" data-name="Button">
      <Container19 />
    </div>
  );
}

function ButtonMargin2() {
  return (
    <div className="content-stretch flex flex-col h-[24px] items-start pl-[4px] relative shrink-0 w-[28px]" data-name="Button:margin">
      <Button5 />
    </div>
  );
}

function Margin10() {
  return (
    <div className="content-stretch flex flex-col h-[16px] items-start pl-[4px] relative shrink-0 w-[5px]" data-name="Margin">
      <div className="bg-[#31394d] h-[16px] shrink-0 w-px" data-name="Vertical Divider" />
    </div>
  );
}

function Container20() {
  return (
    <div className="h-[8px] relative shrink-0 w-[12px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 12 8">
        <g id="Container">
          <path d={svgPaths.p2a605b80} fill="var(--fill-0, #88919D)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Button6() {
  return (
    <div className="content-stretch flex items-center justify-center relative rounded-[4px] shrink-0 size-[24px]" data-name="Button">
      <Container20 />
    </div>
  );
}

function ButtonMargin3() {
  return (
    <div className="content-stretch flex flex-col h-[24px] items-start pl-[4px] relative shrink-0 w-[28px]" data-name="Button:margin">
      <Button6 />
    </div>
  );
}

function Container21() {
  return (
    <div className="relative shrink-0 size-[12px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 12 12">
        <g id="Container">
          <path d={svgPaths.p224ad400} fill="var(--fill-0, #88919D)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Button7() {
  return (
    <div className="content-stretch flex items-center justify-center relative rounded-[4px] shrink-0 size-[24px]" data-name="Button">
      <Container21 />
    </div>
  );
}

function ButtonMargin4() {
  return (
    <div className="content-stretch flex flex-col h-[24px] items-start pl-[4px] relative shrink-0 w-[28px]" data-name="Button:margin">
      <Button7 />
    </div>
  );
}

function Container17() {
  return (
    <div className="relative shrink-0" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex items-center relative size-full">
        <Button4 />
        <ButtonMargin2 />
        <Margin10 />
        <ButtonMargin3 />
        <ButtonMargin4 />
      </div>
    </div>
  );
}

function PanelHeader() {
  return (
    <div className="bg-[#131b2e] h-[40px] relative shrink-0 w-full" data-name="Panel Header">
      <div aria-hidden="true" className="absolute border-[#31394d] border-b border-solid inset-0 pointer-events-none" />
      <div className="flex flex-row items-center size-full">
        <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex items-center justify-between pb-px px-[16px] relative size-full">
          <Container15 />
          <Container17 />
        </div>
      </div>
    </div>
  );
}

function Svg() {
  return (
    <div className="absolute h-[539.8px] left-0 top-0 w-[839px]" data-name="SVG">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 839 539.8">
        <g id="SVG">
          <g id="Vector">
            <path d={svgPaths.p1299b400} fill="var(--fill-0, black)" />
            <path d={svgPaths.p1299b400} stroke="var(--stroke-0, #00A3FF)" strokeDasharray="4 2" strokeOpacity="0.6" strokeWidth="1.5" />
          </g>
          <g id="Vector_2">
            <path d={svgPaths.p3089d900} fill="var(--fill-0, black)" />
            <path d={svgPaths.p3089d900} stroke="var(--stroke-0, #13FF43)" strokeOpacity="0.4" />
          </g>
          <g id="Vector_3">
            <path d={svgPaths.pd725680} fill="var(--fill-0, black)" />
            <path d={svgPaths.pd725680} stroke="var(--stroke-0, #3F4852)" />
          </g>
          <g id="Vector_4">
            <path d={svgPaths.p27a8a200} fill="var(--fill-0, black)" />
            <path d={svgPaths.p27a8a200} stroke="var(--stroke-0, #FFB4AB)" strokeOpacity="0.8" strokeWidth="2" />
          </g>
          <g filter="url(#filter0_f_1_372)" id="Vector_5">
            <path d={svgPaths.p32687f00} fill="var(--fill-0, #00A3FF)" />
          </g>
        </g>
        <defs>
          <filter colorInterpolationFilters="sRGB" filterUnits="userSpaceOnUse" height="10" id="filter0_f_1_372" width="10" x="288.65" y="210.92">
            <feFlood floodOpacity="0" result="BackgroundImageFix" />
            <feBlend in="SourceGraphic" in2="BackgroundImageFix" mode="normal" result="shape" />
            <feGaussianBlur result="effect1_foregroundBlur_1_372" stdDeviation="1" />
          </filter>
        </defs>
      </svg>
    </div>
  );
}

function Border1() {
  return (
    <div className="flex-[1_0_0] min-h-px relative w-full" data-name="Border">
      <div aria-hidden="true" className="absolute border border-[rgba(63,72,82,0.3)] border-solid inset-0 pointer-events-none" />
      <div className="bg-clip-padding border-0 border-[transparent] border-solid relative size-full">
        <div className="absolute bg-[#3f4852] inset-[30.44%_76.11%_64.91%_20.5%] rounded-[9999px]" data-name="Background" />
        <div className="-translate-x-1/2 -translate-y-1/2 absolute bg-[#00a3ff] left-[calc(50%+3px)] rounded-[9999px] size-[6px] top-[calc(50%+3px)]" data-name="Background" />
        <div className="absolute bg-[#13ff43] inset-[40.22%_17.12%_55.13%_79.49%] rounded-[9999px]" data-name="Background" />
        <div className="absolute bg-[#3f4852] inset-[79.28%_56.45%_16.07%_40.16%] rounded-[9999px]" data-name="Background" />
        <div className="absolute bg-[#ffb4ab] inset-[69.52%_26.96%_25.83%_69.65%] rounded-[9999px]" data-name="Background" />
        <div className="absolute bg-[rgba(0,163,255,0.1)] border border-[rgba(0,163,255,0.5)] border-solid inset-[10.92%_10.69%_10.95%_10.67%]" data-name="Viewport box" />
      </div>
    </div>
  );
}

function Minimap() {
  return (
    <div className="absolute bg-[#171f33] bottom-[16px] content-stretch flex flex-col h-[96px] items-start justify-center opacity-50 p-[5px] right-[16px] rounded-[4px] w-[128px]" data-name="Minimap">
      <div aria-hidden="true" className="absolute border border-[#31394d] border-solid inset-0 pointer-events-none rounded-[4px]" />
      <Border1 />
    </div>
  );
}

function Container22() {
  return (
    <div className="h-[19px] relative shrink-0 w-[18.65px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 18.65 19">
        <g id="Container">
          <path d={svgPaths.p521cb00} fill="var(--fill-0, #00A3FF)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function BackgroundBorderShadow() {
  return (
    <div className="bg-[#171f33] content-stretch flex items-center justify-center p-[2px] relative rounded-[9999px] shrink-0 size-[48px]" data-name="Background+Border+Shadow">
      <div aria-hidden="true" className="absolute border-2 border-[#00a3ff] border-solid inset-0 pointer-events-none rounded-[9999px] shadow-[0px_0px_15px_0px_rgba(0,163,255,0.3)]" />
      <Container22 />
    </div>
  );
}

function BackgroundBorder1() {
  return (
    <div className="bg-[#0b1326] content-stretch flex flex-col items-start px-[7px] py-[3px] relative rounded-[4px] shrink-0" data-name="Background+Border">
      <div aria-hidden="true" className="absolute border border-[#3f4852] border-solid inset-0 pointer-events-none rounded-[4px]" />
      <div className="flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] relative shrink-0 text-[#dae2fd] text-[14px] tracking-[0.7px] w-[54.86px]">
        <p className="leading-[20px]">10.0.0.1</p>
      </div>
    </div>
  );
}

function Margin11() {
  return (
    <div className="content-stretch flex flex-col items-start pt-[4px] relative shrink-0" data-name="Margin">
      <BackgroundBorder1 />
    </div>
  );
}

function CoreSwitch() {
  return (
    <div className="absolute content-stretch flex flex-col inset-[42.77%_45.9%_42.78%_45.9%] items-center" data-name="Core Switch">
      <BackgroundBorderShadow />
      <Margin11 />
    </div>
  );
}

function Container23() {
  return (
    <div className="relative shrink-0 size-[13.333px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 13.3333 13.3333">
        <g id="Container">
          <path d={svgPaths.p249612c0} fill="var(--fill-0, #3F4852)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function BackgroundBorder2() {
  return (
    <div className="bg-[#171f33] content-stretch flex items-center justify-center p-px relative rounded-[9999px] shrink-0 size-[32px]" data-name="Background+Border">
      <div aria-hidden="true" className="absolute border border-[#88919d] border-solid inset-0 pointer-events-none rounded-[9999px]" />
      <Container23 />
    </div>
  );
}

function BackgroundBorder3() {
  return (
    <div className="bg-[#0b1326] content-stretch flex flex-col items-start px-[7px] py-[3px] relative rounded-[4px] shrink-0" data-name="Background+Border">
      <div aria-hidden="true" className="absolute border border-[#3f4852] border-solid inset-0 pointer-events-none rounded-[4px]" />
      <div className="flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] relative shrink-0 text-[#bec7d4] text-[14px] tracking-[0.7px] w-[78.8px]">
        <p className="leading-[20px]">192.168.1.5</p>
      </div>
    </div>
  );
}

function Margin12() {
  return (
    <div className="content-stretch flex flex-col items-start pt-[4px] relative shrink-0" data-name="Margin">
      <BackgroundBorder3 />
    </div>
  );
}

function ExternalNode() {
  return (
    <div className="absolute content-stretch flex flex-col inset-[24.26%_74.47%_64.26%_14.47%] items-center" data-name="External Node">
      <BackgroundBorder2 />
      <Margin12 />
    </div>
  );
}

function Container24() {
  return (
    <div className="h-[12.667px] relative shrink-0 w-[12px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 12 12.6667">
        <g id="Container">
          <path d={svgPaths.pf3d7880} fill="var(--fill-0, #13FF43)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function BackgroundBorder4() {
  return (
    <div className="bg-[#171f33] content-stretch flex items-center justify-center p-[2px] relative rounded-[9999px] shrink-0 size-[32px]" data-name="Background+Border">
      <div aria-hidden="true" className="absolute border-2 border-[#13ff43] border-solid inset-0 pointer-events-none rounded-[9999px]" />
      <Container24 />
    </div>
  );
}

function BackgroundBorder5() {
  return (
    <div className="bg-[#0b1326] content-stretch flex flex-col items-start px-[7px] py-[3px] relative rounded-[4px] shrink-0" data-name="Background+Border">
      <div aria-hidden="true" className="absolute border border-[#3f4852] border-solid inset-0 pointer-events-none rounded-[4px]" />
      <div className="flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] relative shrink-0 text-[#13ff43] text-[14px] tracking-[0.7px] w-[64.11px]">
        <p className="leading-[20px]">10.0.2.14</p>
      </div>
    </div>
  );
}

function Margin13() {
  return (
    <div className="content-stretch flex flex-col items-start pt-[4px] relative shrink-0" data-name="Margin">
      <BackgroundBorder5 />
    </div>
  );
}

function TargetNode() {
  return (
    <div className="absolute content-stretch flex flex-col inset-[34.26%_15.35%_54.26%_75.34%] items-center" data-name="Target Node">
      <BackgroundBorder4 />
      <Margin13 />
    </div>
  );
}

function Container25() {
  return (
    <div className="h-[10.667px] relative shrink-0 w-[12px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 12 10.6667">
        <g id="Container">
          <path d={svgPaths.p3d13b6c0} fill="var(--fill-0, #3F4852)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function BackgroundBorder6() {
  return (
    <div className="bg-[#171f33] content-stretch flex items-center justify-center p-px relative rounded-[9999px] shrink-0 size-[32px]" data-name="Background+Border">
      <div aria-hidden="true" className="absolute border border-[#88919d] border-solid inset-0 pointer-events-none rounded-[9999px]" />
      <Container25 />
    </div>
  );
}

function BackgroundBorder7() {
  return (
    <div className="bg-[#0b1326] content-stretch flex flex-col items-start px-[7px] py-[3px] relative rounded-[4px] shrink-0" data-name="Background+Border">
      <div aria-hidden="true" className="absolute border border-[#3f4852] border-solid inset-0 pointer-events-none rounded-[4px]" />
      <div className="flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] relative shrink-0 text-[#bec7d4] text-[14px] tracking-[0.7px] w-[66.73px]">
        <p className="leading-[20px]">10.0.3.55</p>
      </div>
    </div>
  );
}

function Margin14() {
  return (
    <div className="content-stretch flex flex-col items-start pt-[4px] relative shrink-0" data-name="Margin">
      <BackgroundBorder7 />
    </div>
  );
}

function DatabaseNode() {
  return (
    <div className="absolute content-stretch flex flex-col inset-[74.26%_55.19%_14.26%_35.19%] items-center" data-name="Database Node">
      <BackgroundBorder6 />
      <Margin14 />
    </div>
  );
}

function Container26() {
  return (
    <div className="h-[12.667px] relative shrink-0 w-[14.667px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 14.6667 12.6667">
        <g id="Container">
          <path d={svgPaths.pc531a80} fill="var(--fill-0, #FFB4AB)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function BackgroundBorderShadow1() {
  return (
    <div className="bg-[#171f33] content-stretch flex items-center justify-center p-[2px] relative rounded-[9999px] shrink-0 size-[32px]" data-name="Background+Border+Shadow">
      <div aria-hidden="true" className="absolute border-2 border-[#ffb4ab] border-solid inset-0 pointer-events-none rounded-[9999px] shadow-[0px_0px_10px_0px_rgba(255,180,171,0.4)]" />
      <Container26 />
    </div>
  );
}

function BackgroundBorder8() {
  return (
    <div className="bg-[#0b1326] content-stretch flex flex-col items-start px-[7px] py-[3px] relative rounded-[4px] shrink-0" data-name="Background+Border">
      <div aria-hidden="true" className="absolute border border-[#ffb4ab] border-solid inset-0 pointer-events-none rounded-[4px]" />
      <div className="flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] relative shrink-0 text-[#ffb4ab] text-[14px] tracking-[0.7px] w-[67.14px]">
        <p className="leading-[20px]">10.0.9.99</p>
      </div>
    </div>
  );
}

function Margin15() {
  return (
    <div className="content-stretch flex flex-col items-start pt-[4px] relative shrink-0" data-name="Margin">
      <BackgroundBorder8 />
    </div>
  );
}

function AlertNode() {
  return (
    <div className="absolute content-stretch flex flex-col inset-[64.25%_25.16%_24.26%_65.16%] items-center" data-name="Alert Node">
      <BackgroundBorderShadow1 />
      <Margin15 />
    </div>
  );
}

function TopologyCanvas() {
  return (
    <div className="bg-[#060e20] flex-[1_0_0] min-h-px relative w-full" data-name="Topology Canvas">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid overflow-clip relative rounded-[inherit] size-full">
        <div className="absolute inset-0 opacity-10" style={{ backgroundImage: "url('data:image/svg+xml;utf8,<svg viewBox=\\'0 0 839 539.8\\' xmlns=\\'http://www.w3.org/2000/svg\\' preserveAspectRatio=\\'none\\'><rect x=\\'0\\' y=\\'0\\' height=\\'100%\\' width=\\'100%\\' fill=\\'url(%23grad)\\' opacity=\\'1\\'/><defs><radialGradient id=\\'grad\\' gradientUnits=\\'userSpaceOnUse\\' cx=\\'0\\' cy=\\'0\\' r=\\'10\\' gradientTransform=\\'matrix(59.326 0 0 38.17 419.5 269.9)\\'><stop stop-color=\\'rgba(63,72,82,1)\\' offset=\\'0.035355\\'/><stop stop-color=\\'rgba(63,72,82,0)\\' offset=\\'0.035355\\'/></radialGradient></defs></svg>')" }} data-name="Background Grid Pattern" />
        <Svg />
        <Minimap />
        <CoreSwitch />
        <ExternalNode />
        <TargetNode />
        <DatabaseNode />
        <AlertNode />
      </div>
    </div>
  );
}

function TopLeftPaneNetworkTopology() {
  return (
    <div className="h-[580.8px] relative shrink-0 w-full" data-name="Top-Left Pane: Network Topology (60%)">
      <div aria-hidden="true" className="absolute border-[#31394d] border-b border-solid inset-0 pointer-events-none" />
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-start pb-px relative size-full">
        <PanelHeader />
        <TopologyCanvas />
      </div>
    </div>
  );
}

function Container28() {
  return (
    <div className="relative shrink-0 size-[12px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 12 12">
        <g id="Container">
          <path d={svgPaths.p3a561800} fill="var(--fill-0, #BEC7D4)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Heading2Margin1() {
  return (
    <div className="content-stretch flex flex-col items-start pl-[8px] relative shrink-0" data-name="Heading 2:margin">
      <div className="flex flex-col font-['Space_Grotesk:Bold',sans-serif] font-bold h-[16px] justify-center leading-[0] relative shrink-0 text-[#dae2fd] text-[11px] tracking-[0.88px] w-[138.33px]">
        <p className="leading-[16px]">TRAFFIC_METRICS_PPS</p>
      </div>
    </div>
  );
}

function Container27() {
  return (
    <div className="relative shrink-0" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex items-center relative size-full">
        <Container28 />
        <Heading2Margin1 />
      </div>
    </div>
  );
}

function Button8() {
  return (
    <div className="bg-[#171f33] content-stretch flex flex-col items-center justify-center px-[9px] py-[3px] relative rounded-[4px] shrink-0" data-name="Button">
      <div aria-hidden="true" className="absolute border border-[#31394d] border-solid inset-0 pointer-events-none rounded-[4px]" />
      <div className="flex flex-col font-['Space_Grotesk:Regular',sans-serif] font-normal h-[22px] justify-center leading-[0] relative shrink-0 text-[#3f4852] text-[9px] text-center w-[11.52px]">
        <p className="leading-[22px]">1m</p>
      </div>
    </div>
  );
}

function Button9() {
  return (
    <div className="bg-[rgba(0,163,255,0.1)] content-stretch flex flex-col items-center justify-center px-[9px] py-[3px] relative rounded-[4px] shrink-0" data-name="Button">
      <div aria-hidden="true" className="absolute border border-[rgba(0,163,255,0.5)] border-solid inset-0 pointer-events-none rounded-[4px]" />
      <div className="flex flex-col font-['Space_Grotesk:Regular',sans-serif] font-normal h-[22px] justify-center leading-[0] relative shrink-0 text-[#00a3ff] text-[9px] text-center w-[13.14px]">
        <p className="leading-[22px]">5m</p>
      </div>
    </div>
  );
}

function ButtonMargin5() {
  return (
    <div className="content-stretch flex flex-col items-start pl-[4px] relative shrink-0" data-name="Button:margin">
      <Button9 />
    </div>
  );
}

function Button10() {
  return (
    <div className="bg-[#171f33] content-stretch flex flex-col items-center justify-center px-[9px] py-[3px] relative rounded-[4px] shrink-0" data-name="Button">
      <div aria-hidden="true" className="absolute border border-[#31394d] border-solid inset-0 pointer-events-none rounded-[4px]" />
      <div className="flex flex-col font-['Space_Grotesk:Regular',sans-serif] font-normal h-[22px] justify-center leading-[0] relative shrink-0 text-[#3f4852] text-[9px] text-center w-[9.28px]">
        <p className="leading-[22px]">1h</p>
      </div>
    </div>
  );
}

function ButtonMargin6() {
  return (
    <div className="content-stretch flex flex-col items-start pl-[4px] relative shrink-0" data-name="Button:margin">
      <Button10 />
    </div>
  );
}

function Container30() {
  return (
    <div className="content-stretch flex items-start relative shrink-0" data-name="Container">
      <Button8 />
      <ButtonMargin5 />
      <ButtonMargin6 />
    </div>
  );
}

function Margin16() {
  return (
    <div className="content-stretch flex flex-col h-[16px] items-start pl-[8px] relative shrink-0 w-[9px]" data-name="Margin">
      <div className="bg-[#31394d] h-[16px] shrink-0 w-px" data-name="Vertical Divider" />
    </div>
  );
}

function Container31() {
  return (
    <div className="relative shrink-0 size-[10.667px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 10.6667 10.6667">
        <g id="Container">
          <path d={svgPaths.p358da480} fill="var(--fill-0, #88919D)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Button11() {
  return (
    <div className="content-stretch flex items-center justify-center relative rounded-[4px] shrink-0 size-[24px]" data-name="Button">
      <Container31 />
    </div>
  );
}

function ButtonMargin7() {
  return (
    <div className="content-stretch flex flex-col h-[24px] items-start pl-[8px] relative shrink-0 w-[32px]" data-name="Button:margin">
      <Button11 />
    </div>
  );
}

function Container29() {
  return (
    <div className="relative shrink-0" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex items-center relative size-full">
        <Container30 />
        <Margin16 />
        <ButtonMargin7 />
      </div>
    </div>
  );
}

function PanelHeader1() {
  return (
    <div className="bg-[#131b2e] h-[40px] relative shrink-0 w-full" data-name="Panel Header">
      <div aria-hidden="true" className="absolute border-[#31394d] border-b border-solid inset-0 pointer-events-none" />
      <div className="flex flex-row items-center size-full">
        <div className="content-stretch flex items-center justify-between pb-px px-[16px] relative size-full">
          <Container27 />
          <Container29 />
        </div>
      </div>
    </div>
  );
}

function Container32() {
  return (
    <div className="relative shrink-0 w-full" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-end relative size-full">
        <div className="flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] relative shrink-0 text-[#3f4852] text-[14px] text-right tracking-[0.7px] w-[24.94px]">
          <p className="leading-[20px]">10k</p>
        </div>
      </div>
    </div>
  );
}

function Container33() {
  return (
    <div className="h-[20px] relative shrink-0 w-full" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid relative size-full">
        <div className="-translate-y-1/2 absolute flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] right-[-2.66px] text-[#3f4852] text-[14px] text-right top-[10px] tracking-[0.7px] w-[29.66px]">
          <p className="leading-[20px]">7.5k</p>
        </div>
      </div>
    </div>
  );
}

function Container34() {
  return (
    <div className="relative shrink-0 w-full" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-end relative size-full">
        <div className="flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] relative shrink-0 text-[#3f4852] text-[14px] text-right tracking-[0.7px] w-[17.5px]">
          <p className="leading-[20px]">5k</p>
        </div>
      </div>
    </div>
  );
}

function Container35() {
  return (
    <div className="h-[20px] relative shrink-0 w-full" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid relative size-full">
        <div className="-translate-y-1/2 absolute flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] right-[-4.14px] text-[#3f4852] text-[14px] text-right top-[10px] tracking-[0.7px] w-[31.14px]">
          <p className="leading-[20px]">2.5k</p>
        </div>
      </div>
    </div>
  );
}

function Container36() {
  return (
    <div className="relative shrink-0 w-full" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-end relative size-full">
        <div className="flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] relative shrink-0 text-[#3f4852] text-[14px] text-right tracking-[0.7px] w-[9.73px]">
          <p className="leading-[20px]">0</p>
        </div>
      </div>
    </div>
  );
}

function YAxisLabels() {
  return (
    <div className="absolute bottom-[32px] content-stretch flex flex-col items-start justify-between left-[16px] pb-[0.01px] pr-[5px] top-[16px] w-[32px]" data-name="Y-axis labels">
      <div aria-hidden="true" className="absolute border-[#31394d] border-r border-solid inset-0 pointer-events-none" />
      <Container32 />
      <Container33 />
      <Container34 />
      <Container35 />
      <Container36 />
    </div>
  );
}

function Container37() {
  return (
    <div className="h-full relative shrink-0 w-[63.72px]" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid relative size-full">
        <div className="-translate-y-1/2 absolute flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] left-0 text-[#3f4852] text-[14px] top-[10px] tracking-[0.7px] w-[63.72px]">
          <p className="leading-[20px]">10:45:00</p>
        </div>
      </div>
    </div>
  );
}

function Container38() {
  return (
    <div className="h-full relative shrink-0 w-[63.95px]" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid relative size-full">
        <div className="-translate-y-1/2 absolute flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] left-0 text-[#3f4852] text-[14px] top-[10px] tracking-[0.7px] w-[63.95px]">
          <p className="leading-[20px]">10:46:00</p>
        </div>
      </div>
    </div>
  );
}

function Container39() {
  return (
    <div className="h-full relative shrink-0 w-[62.64px]" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid relative size-full">
        <div className="-translate-y-1/2 absolute flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] left-0 text-[#3f4852] text-[14px] top-[10px] tracking-[0.7px] w-[62.64px]">
          <p className="leading-[20px]">10:47:00</p>
        </div>
      </div>
    </div>
  );
}

function Container40() {
  return (
    <div className="h-full relative shrink-0 w-[63.89px]" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid relative size-full">
        <div className="-translate-y-1/2 absolute flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] left-0 text-[#3f4852] text-[14px] top-[10px] tracking-[0.7px] w-[63.89px]">
          <p className="leading-[20px]">10:48:00</p>
        </div>
      </div>
    </div>
  );
}

function Container41() {
  return (
    <div className="h-full relative shrink-0 w-[63.95px]" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid relative size-full">
        <div className="-translate-y-1/2 absolute flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] left-0 text-[#3f4852] text-[14px] top-[10px] tracking-[0.7px] w-[63.95px]">
          <p className="leading-[20px]">10:49:00</p>
        </div>
      </div>
    </div>
  );
}

function Container42() {
  return (
    <div className="h-full relative shrink-0 w-[33.19px]" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid relative size-full">
        <div className="-translate-y-1/2 absolute flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] left-0 text-[#3f4852] text-[14px] top-[10px] tracking-[0.7px] w-[33.19px]">
          <p className="leading-[20px]">NOW</p>
        </div>
      </div>
    </div>
  );
}

function XAxisLabels() {
  return (
    <div className="absolute bottom-[8.01px] content-stretch flex h-[16px] items-start justify-between left-[48px] pr-[0.06px] pt-[5px] right-[16px]" data-name="X-axis labels">
      <div aria-hidden="true" className="absolute border-[#31394d] border-solid border-t inset-0 pointer-events-none" />
      <Container37 />
      <Container38 />
      <Container39 />
      <Container40 />
      <Container41 />
      <Container42 />
    </div>
  );
}

function GridLines() {
  return (
    <div className="absolute content-stretch flex flex-col h-[299.19px] items-start justify-between left-0 pb-[0.01px] top-0 w-[775px]" data-name="Grid lines">
      <div className="bg-[rgba(49,57,77,0.5)] h-px shrink-0 w-full" data-name="Horizontal Divider" />
      <div className="bg-[rgba(49,57,77,0.5)] h-px shrink-0 w-full" data-name="Horizontal Divider" />
      <div className="bg-[rgba(49,57,77,0.5)] h-px shrink-0 w-full" data-name="Horizontal Divider" />
      <div className="bg-[rgba(49,57,77,0.5)] h-px shrink-0 w-full" data-name="Horizontal Divider" />
      <div className="h-px shrink-0 w-full" data-name="Rectangle" />
    </div>
  );
}

function Svg1() {
  return (
    <div className="absolute h-[299.19px] left-0 top-0 w-[775px]" data-name="SVG">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 775 299.19">
        <g clipPath="url(#clip0_1_397)" id="SVG">
          <path d={svgPaths.p380c7800} id="Vector" stroke="var(--stroke-0, #00A3FF)" strokeWidth="8.05643" />
          <path d={svgPaths.p379ba400} fill="url(#paint0_linear_1_397)" id="Vector_2" opacity="0.1" />
        </g>
        <defs>
          <linearGradient gradientUnits="userSpaceOnUse" id="paint0_linear_1_397" x1="0" x2="0" y1="29.919" y2="299.19">
            <stop stopColor="#00A3FF" />
            <stop offset="1" stopOpacity="0" />
          </linearGradient>
          <clipPath id="clip0_1_397">
            <rect fill="white" height="299.19" width="775" />
          </clipPath>
        </defs>
      </svg>
    </div>
  );
}

function Svg2() {
  return (
    <div className="absolute h-[299.19px] left-0 top-0 w-[775px]" data-name="SVG">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 775 299.19">
        <g clipPath="url(#clip0_1_348)" id="SVG">
          <path d={svgPaths.p423e6d8} id="Vector" stroke="var(--stroke-0, #FFB4AB)" strokeDasharray="10.74 10.74" strokeWidth="5.37095" />
        </g>
        <defs>
          <clipPath id="clip0_1_348">
            <rect fill="white" height="299.19" width="775" />
          </clipPath>
        </defs>
      </svg>
    </div>
  );
}

function Background() {
  return (
    <div className="bg-[#00a3ff] content-stretch flex flex-col items-start justify-center relative rounded-[9999px] shrink-0 size-[8px]" data-name="Background">
      <div className="bg-[rgba(255,255,255,0)] rounded-[9999px] shadow-[0px_0px_0px_2px_#0b1326,0px_0px_8px_0px_rgba(0,163,255,0.8)] shrink-0 size-[8px]" data-name="Overlay+Shadow" />
    </div>
  );
}

function BackgroundBorder9() {
  return (
    <div className="absolute bg-[#222a3d] content-stretch flex flex-col items-start left-[12px] px-[7px] py-[3px] rounded-[4px] top-[-9px]" data-name="Background+Border">
      <div aria-hidden="true" className="absolute border border-[#3f4852] border-solid inset-0 pointer-events-none rounded-[4px]" />
      <div className="flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] relative shrink-0 text-[#dae2fd] text-[14px] tracking-[0.7px] w-[73.02px]">
        <p className="leading-[20px]">8,452 pps</p>
      </div>
    </div>
  );
}

function CurrentValueMarker() {
  return (
    <div className="absolute content-stretch flex h-[8px] items-center right-[-4px] top-[44.87px]" data-name="Current value marker">
      <Background />
      <BackgroundBorder9 />
    </div>
  );
}

function ChartCanvasSimulatedWithSvg() {
  return (
    <div className="absolute inset-[16px_16px_32px_48px]" data-name="Chart Canvas (Simulated with SVG)">
      <GridLines />
      <Svg1 />
      <Svg2 />
      <CurrentValueMarker />
    </div>
  );
}

function GraphAreaSimulated() {
  return (
    <div className="bg-[#060e20] flex-[1_0_0] min-h-px overflow-clip relative w-full" data-name="Graph Area (Simulated)">
      <YAxisLabels />
      <XAxisLabels />
      <ChartCanvasSimulatedWithSvg />
    </div>
  );
}

function BottomLeftPaneMetricsGraph() {
  return (
    <div className="h-[387.19px] relative shrink-0 w-full" data-name="Bottom-Left Pane: Metrics Graph (40%)">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-start relative size-full">
        <PanelHeader1 />
        <GraphAreaSimulated />
      </div>
    </div>
  );
}

function LeftColumnDataVisualization() {
  return (
    <div className="content-stretch flex flex-col h-full items-start pr-px relative shrink-0 w-[840px]" data-name="Left Column: Data Visualization (70%)">
      <div aria-hidden="true" className="absolute border-[#31394d] border-r border-solid inset-0 pointer-events-none" />
      <TopLeftPaneNetworkTopology />
      <BottomLeftPaneMetricsGraph />
    </div>
  );
}

function Container44() {
  return (
    <div className="h-[12.667px] relative shrink-0 w-[14.667px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 14.6667 12.6667">
        <g id="Container">
          <path d={svgPaths.p11254080} fill="var(--fill-0, #00E639)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Heading2Margin2() {
  return (
    <div className="content-stretch flex flex-col items-start pl-[8px] relative shrink-0" data-name="Heading 2:margin">
      <div className="flex flex-col font-['Space_Grotesk:Bold',sans-serif] font-bold h-[16px] justify-center leading-[0] relative shrink-0 text-[#dae2fd] text-[11px] tracking-[0.88px] w-[156.61px]">
        <p className="leading-[16px]">NET_VISOR_AI_ASSISTANT</p>
      </div>
    </div>
  );
}

function Container43() {
  return (
    <div className="relative shrink-0" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex items-center relative size-full">
        <Container44 />
        <Heading2Margin2 />
      </div>
    </div>
  );
}

function Container45() {
  return (
    <div className="h-[9.667px] relative shrink-0 w-[13.333px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 13.3333 9.66667">
        <g id="Container">
          <path d={svgPaths.pa72eee0} fill="var(--fill-0, #88919D)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Button12() {
  return (
    <div className="relative shrink-0" data-name="Button">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-center justify-center relative size-full">
        <Container45 />
      </div>
    </div>
  );
}

function PanelHeader2() {
  return (
    <div className="bg-[#131b2e] h-[40px] relative shrink-0 w-full" data-name="Panel Header">
      <div aria-hidden="true" className="absolute border-[#31394d] border-b border-solid inset-0 pointer-events-none shadow-[0px_1px_0px_0px_rgba(255,255,255,0.05)]" />
      <div className="flex flex-row items-center size-full">
        <div className="content-stretch flex items-center justify-between pb-px px-[16px] relative size-full">
          <Container43 />
          <Button12 />
        </div>
      </div>
    </div>
  );
}

function BackgroundBorder10() {
  return (
    <div className="bg-[#171f33] relative rounded-[4px] self-stretch shrink-0" data-name="Background+Border">
      <div aria-hidden="true" className="absolute border border-[#31394d] border-solid inset-0 pointer-events-none rounded-[4px]" />
      <div className="content-stretch flex flex-col items-start px-[9px] py-[3px] relative size-full">
        <div className="flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] relative shrink-0 text-[#3f4852] text-[14px] tracking-[0.7px] w-[227.53px]">
          <p className="leading-[20px]">{`SESSION_START // 10:42:15 UTC`}</p>
        </div>
      </div>
    </div>
  );
}

function Timestamp() {
  return (
    <div className="content-stretch flex h-[26px] items-start justify-center relative shrink-0 w-full" data-name="Timestamp">
      <BackgroundBorder10 />
    </div>
  );
}

function Container47() {
  return (
    <div className="content-stretch flex flex-col items-start mr-[-0.01px] relative shrink-0" data-name="Container">
      <div className="flex flex-col font-['Space_Grotesk:Bold',sans-serif] font-bold h-[16px] justify-center leading-[0] relative shrink-0 text-[#3f4852] text-[11px] tracking-[0.88px] w-[82.55px]">
        <p className="leading-[16px]">OPERATOR_01</p>
      </div>
    </div>
  );
}

function Margin18() {
  return (
    <div className="h-[9.333px] mr-[-0.01px] relative shrink-0 w-[17.333px]" data-name="Margin">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 17.3333 9.33333">
        <g id="Margin">
          <path d={svgPaths.p37412280} fill="var(--fill-0, #3F4852)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Container46() {
  return (
    <div className="content-stretch flex items-center pr-[0.01px] relative shrink-0" data-name="Container">
      <Container47 />
      <Margin18 />
    </div>
  );
}

function Margin17() {
  return (
    <div className="content-stretch flex flex-col items-start pb-[4px] relative shrink-0" data-name="Margin">
      <Container46 />
    </div>
  );
}

function Background1() {
  return (
    <div className="absolute bg-[#2d3449] h-[18px] left-[13px] rounded-[4px] top-[37px] w-[86.8px]" data-name="Background">
      <div className="-translate-y-1/2 absolute flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] left-[4px] text-[#00a3ff] text-[14px] top-[9px] tracking-[0.7px] w-[78.8px]">
        <p className="leading-[20px]">192.168.1.5</p>
      </div>
    </div>
  );
}

function Background2() {
  return (
    <div className="absolute bg-[#2d3449] h-[18px] left-[120.52px] rounded-[4px] top-[37px] w-[72.11px]" data-name="Background">
      <div className="-translate-y-1/2 absolute flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] left-[4px] text-[#13ff43] text-[14px] top-[9px] tracking-[0.7px] w-[64.11px]">
        <p className="leading-[20px]">10.0.2.14</p>
      </div>
    </div>
  );
}

function BackgroundBorder11() {
  return (
    <div className="bg-[#171f33] h-[92px] max-w-[278.79998779296875px] relative rounded-bl-[4px] rounded-br-[4px] rounded-tl-[4px] shrink-0 w-[278.8px]" data-name="Background+Border">
      <div aria-hidden="true" className="absolute border border-[#31394d] border-solid inset-0 pointer-events-none rounded-bl-[4px] rounded-br-[4px] rounded-tl-[4px]" />
      <div className="-translate-y-1/2 absolute flex flex-col font-['Inter:Regular',sans-serif] font-normal h-[22px] justify-center leading-[0] left-[13px] not-italic text-[#dae2fd] text-[14px] top-[23.5px] w-[209.86px]">
        <p className="leading-[22px]">Analyze the spike in traffic from</p>
      </div>
      <Background1 />
      <div className="-translate-y-1/2 absolute flex flex-col font-['Inter:Regular',sans-serif] font-normal h-[22px] justify-center leading-[0] left-[99.8px] not-italic text-[#dae2fd] text-[14px] top-[45.5px] w-[20.72px]">
        <p className="leading-[22px]">{` to `}</p>
      </div>
      <Background2 />
      <div className="-translate-y-1/2 absolute flex flex-col font-['Inter:Regular',sans-serif] font-normal h-[22px] justify-center leading-[0] left-[192.63px] not-italic text-[#dae2fd] text-[14px] top-[45.5px] w-[58.05px]">
        <p className="leading-[22px]">{` over the`}</p>
      </div>
      <div className="-translate-y-1/2 absolute flex flex-col font-['Inter:Regular',sans-serif] font-normal h-[22px] justify-center leading-[0] left-[13px] not-italic text-[#dae2fd] text-[14px] top-[67.5px] w-[95.64px]">
        <p className="leading-[22px]">last 5 minutes.</p>
      </div>
    </div>
  );
}

function Margin19() {
  return (
    <div className="content-stretch flex flex-col items-start max-w-[278.79998779296875px] pt-[4px] relative shrink-0" data-name="Margin">
      <BackgroundBorder11 />
    </div>
  );
}

function OperatorMessage() {
  return (
    <div className="content-stretch flex flex-col items-end relative shrink-0 w-full" data-name="Operator Message">
      <Margin17 />
      <Margin19 />
    </div>
  );
}

function Container49() {
  return (
    <div className="h-[11.083px] relative shrink-0 w-[12.833px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 12.8333 11.0833">
        <g id="Container">
          <path d={svgPaths.p2bfc5c00} fill="var(--fill-0, #00E639)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Margin21() {
  return (
    <div className="content-stretch flex flex-col items-start pl-[8px] relative shrink-0" data-name="Margin">
      <div className="flex flex-col font-['Space_Grotesk:Bold',sans-serif] font-bold h-[16px] justify-center leading-[0] relative shrink-0 text-[#00e639] text-[11px] tracking-[0.88px] w-[80.61px]">
        <p className="leading-[16px]">NET_VISOR AI</p>
      </div>
    </div>
  );
}

function Container48() {
  return (
    <div className="content-stretch flex items-center relative shrink-0" data-name="Container">
      <Container49 />
      <Margin21 />
    </div>
  );
}

function Margin20() {
  return (
    <div className="content-stretch flex flex-col items-start pb-[4px] relative shrink-0" data-name="Margin">
      <Container48 />
    </div>
  );
}

function Paragraph() {
  return (
    <div className="relative shrink-0" data-name="Paragraph">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex items-end leading-[0] pr-[23.31px] relative size-full text-[14px]">
        <div className="flex flex-col font-['Inter:Regular',sans-serif] font-normal h-[22px] justify-center not-italic relative shrink-0 text-[#dae2fd] w-[171.83px]">
          <p className="leading-[22px]">{`Analyzing PCAP segment `}</p>
        </div>
        <div className="flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center relative shrink-0 text-[#3f4852] tracking-[0.7px] w-[78.34px]">
          <p className="leading-[20px]">idx_9942a</p>
        </div>
        <div className="flex flex-col font-['Inter:Regular',sans-serif] font-normal h-[22px] justify-center not-italic relative shrink-0 text-[#dae2fd] w-[12.11px]">
          <p className="leading-[22px]">...</p>
        </div>
      </div>
    </div>
  );
}

function Border2() {
  return (
    <div className="absolute content-stretch flex items-start left-[211.09px] px-[5px] py-px rounded-[4px] top-[2.5px]" data-name="Border">
      <div aria-hidden="true" className="absolute border border-[#88919d] border-solid inset-0 pointer-events-none rounded-[4px]" />
      <div className="flex flex-col font-['Inter:Regular',sans-serif] font-normal h-[16px] justify-center leading-[0] not-italic relative shrink-0 text-[#dae2fd] text-[12px] w-[52.08px]">
        <p className="leading-[16px]">TCP SYN</p>
      </div>
    </div>
  );
}

function Container50() {
  return (
    <div className="h-[66px] relative shrink-0 w-[285.59px]" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid relative size-full">
        <div className="-translate-y-1/2 absolute flex flex-col font-['Inter:Regular',sans-serif] font-normal h-[22px] justify-center leading-[0] left-0 not-italic text-[#dae2fd] text-[14px] top-[10.5px] w-[211.09px]">
          <p className="leading-[22px]">{`I detected a sudden increase in `}</p>
        </div>
        <Border2 />
        <div className="-translate-y-1/2 absolute flex flex-col font-['Inter:Regular',sans-serif] font-normal h-[22px] justify-center leading-[0] left-0 not-italic text-[#dae2fd] text-[14px] top-[32.5px] w-[164.77px]">
          <p className="leading-[22px]">{`packets originating from `}</p>
        </div>
        <div className="-translate-y-1/2 absolute flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] left-[164.77px] text-[#00a3ff] text-[14px] top-[33px] tracking-[0.7px] w-[78.8px]">
          <p className="leading-[20px]">192.168.1.5</p>
        </div>
        <div className="-translate-y-1/2 absolute flex flex-col font-['Inter:Regular',sans-serif] font-normal h-[22px] justify-center leading-[0] left-0 not-italic text-[#dae2fd] text-[14px] top-[54.5px] w-[105.64px]">
          <p className="leading-[22px]">{`directed at port `}</p>
        </div>
        <div className="-translate-y-1/2 absolute flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] left-[105.64px] text-[#88919d] text-[14px] top-[55px] tracking-[0.7px] w-[28.23px]">
          <p className="leading-[20px]">443</p>
        </div>
        <div className="-translate-y-1/2 absolute flex flex-col font-['Inter:Regular',sans-serif] font-normal h-[22px] justify-center leading-[0] left-[133.88px] not-italic text-[#dae2fd] text-[14px] top-[54.5px] w-[24.55px]">
          <p className="leading-[22px]">{` on `}</p>
        </div>
        <div className="-translate-y-1/2 absolute flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center leading-[0] left-[158.42px] text-[#13ff43] text-[14px] top-[55px] tracking-[0.7px] w-[64.11px]">
          <p className="leading-[20px]">10.0.2.14</p>
        </div>
        <div className="-translate-y-1/2 absolute flex flex-col font-['Inter:Regular',sans-serif] font-normal h-[22px] justify-center leading-[0] left-[222.53px] not-italic text-[#dae2fd] text-[14px] top-[54.5px] w-[4.05px]">
          <p className="leading-[22px]">.</p>
        </div>
      </div>
    </div>
  );
}

function EmbeddedCodeDataBlock() {
  return (
    <div className="bg-[#0b1326] relative rounded-[4px] shrink-0" data-name="Embedded Code/Data Block">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col font-['Space_Grotesk:Regular',sans-serif] font-normal items-start leading-[0] overflow-clip pb-[9.375px] pl-[9px] pr-[22.98px] pt-[7.875px] relative rounded-[inherit] size-full text-[11px]">
        <div className="flex flex-col h-[28px] justify-center mb-[-0.5px] relative shrink-0 text-[#bec7d4] w-[249.27px]">
          <p className="leading-[13.75px] mb-0">{`10:45:12.441 IP 192.168.1.5.54321 > 10.0.2.14.443:`}</p>
          <p className="leading-[13.75px]">Flags [S]</p>
        </div>
        <div className="flex flex-col h-[28px] justify-center mb-[-0.5px] relative shrink-0 text-[#bec7d4] w-[253.61px]">
          <p className="leading-[13.75px] mb-0">{`10:45:12.442 IP 192.168.1.5.54322 > 10.0.2.14.443:`}</p>
          <p className="leading-[13.75px]">Flags [S]</p>
        </div>
        <div className="flex flex-col h-[28px] justify-center mb-[-0.5px] relative shrink-0 text-[#bec7d4] w-[253.59px]">
          <p className="leading-[13.75px] mb-0">{`10:45:12.443 IP 192.168.1.5.54323 > 10.0.2.14.443:`}</p>
          <p className="leading-[13.75px]">Flags [S]</p>
        </div>
        <div className="flex flex-col h-[15px] justify-center mb-[-0.5px] relative shrink-0 text-[#88919d] w-[125.19px]">
          <p className="leading-[13.75px]">...[9,402 lines omitted]...</p>
        </div>
      </div>
      <div aria-hidden="true" className="absolute border border-[#31394d] border-solid inset-0 pointer-events-none rounded-[4px]" />
    </div>
  );
}

function Paragraph1() {
  return (
    <div className="h-[110px] relative shrink-0 w-[285.59px]" data-name="Paragraph">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid leading-[0] relative size-full text-[#dae2fd] text-[14px]">
        <div className="-translate-y-1/2 absolute flex flex-col font-['Inter:Bold',sans-serif] font-bold h-[66px] justify-center left-0 not-italic top-[32.5px] w-[251.36px]">
          <p className="mb-0">
            <span className="leading-[22px]">Conclusion:</span>
            <span className="font-['Inter:Regular',sans-serif] font-normal leading-[22px] not-italic">{` This pattern is highly`}</span>
          </p>
          <p className="font-['Inter:Regular',sans-serif] font-normal leading-[22px] mb-0">indicative of a SYN flood attempt. The</p>
          <p className="font-['Inter:Regular',sans-serif] font-normal leading-[22px]">target node (</p>
        </div>
        <div className="-translate-y-1/2 absolute flex flex-col font-['Space_Grotesk:Medium',sans-serif] font-medium h-[20px] justify-center left-[85.27px] top-[55px] tracking-[0.7px] w-[64.11px]">
          <p className="leading-[20px]">10.0.2.14</p>
        </div>
        <div className="-translate-y-1/2 absolute flex flex-col font-['Inter:Regular',sans-serif] font-normal h-[22px] justify-center left-[149.38px] not-italic top-[54.5px] w-[82.91px]">
          <p className="leading-[22px]">) is currently</p>
        </div>
        <div className="-translate-y-1/2 absolute flex flex-col font-['Inter:Regular',sans-serif] font-normal h-[44px] justify-center left-0 not-italic top-[87.5px] w-[269.41px]">
          <p className="leading-[22px] mb-0">dropping ~15% of packets as seen in the</p>
          <p className="leading-[22px]">metrics.</p>
        </div>
      </div>
    </div>
  );
}

function BackgroundBorder12() {
  return (
    <div className="bg-[#222a3d] content-stretch flex flex-col gap-[12px] items-start max-w-[311.6000061035156px] p-[13px] relative rounded-bl-[4px] rounded-br-[4px] rounded-tr-[4px] shrink-0" data-name="Background+Border">
      <div aria-hidden="true" className="absolute border border-[rgba(63,72,82,0.3)] border-solid inset-0 pointer-events-none rounded-bl-[4px] rounded-br-[4px] rounded-tr-[4px]" />
      <Paragraph />
      <Container50 />
      <EmbeddedCodeDataBlock />
      <Paragraph1 />
    </div>
  );
}

function Margin22() {
  return (
    <div className="content-stretch flex flex-col items-start max-w-[311.6000061035156px] pt-[4px] relative shrink-0" data-name="Margin">
      <BackgroundBorder12 />
    </div>
  );
}

function AiResponse() {
  return (
    <div className="content-stretch flex flex-col items-start relative shrink-0 w-full" data-name="AI Response">
      <Margin20 />
      <Margin22 />
    </div>
  );
}

function Button13() {
  return (
    <div className="content-stretch flex flex-col items-center justify-center px-[9px] py-[5px] relative rounded-[4px] shrink-0" data-name="Button">
      <div aria-hidden="true" className="absolute border border-[rgba(0,163,255,0.5)] border-solid inset-0 pointer-events-none rounded-[4px]" />
      <div className="flex flex-col font-['Space_Grotesk:Regular',sans-serif] font-normal h-[16px] justify-center leading-[0] relative shrink-0 text-[#00a3ff] text-[12px] text-center w-[123.33px]">
        <p className="leading-[16px]">BLOCK_IP(192.168.1.5)</p>
      </div>
    </div>
  );
}

function Button14() {
  return (
    <div className="content-stretch flex flex-col items-center justify-center px-[9px] py-[5px] relative rounded-[4px] shrink-0" data-name="Button">
      <div aria-hidden="true" className="absolute border border-[#88919d] border-solid inset-0 pointer-events-none rounded-[4px]" />
      <div className="flex flex-col font-['Space_Grotesk:Regular',sans-serif] font-normal h-[16px] justify-center leading-[0] relative shrink-0 text-[#3f4852] text-[12px] text-center w-[117.77px]">
        <p className="leading-[16px]">VIEW_RAW_PACKETS</p>
      </div>
    </div>
  );
}

function AiSuggestedActionsChips() {
  return (
    <div className="relative shrink-0 w-full" data-name="AI Suggested Actions (Chips)">
      <div className="content-stretch flex gap-[8px] items-start pl-[24px] relative size-full">
        <Button13 />
        <Button14 />
      </div>
    </div>
  );
}

function ChatHistoryArea() {
  return (
    <div className="flex-[1_0_0] min-h-px relative w-full" data-name="Chat History Area">
      <div className="overflow-clip rounded-[inherit] size-full">
        <div className="content-stretch flex flex-col gap-[24px] items-start p-[16px] relative size-full">
          <Timestamp />
          <OperatorMessage />
          <AiResponse />
          <AiSuggestedActionsChips />
        </div>
      </div>
    </div>
  );
}

function Container52() {
  return (
    <div className="flex-[1_0_0] min-w-px relative" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-start relative size-full">
        <div className="flex flex-col font-['Inter:Regular',sans-serif] font-normal justify-center leading-[0] not-italic relative shrink-0 text-[#3f4852] text-[14px] w-full">
          <p className="leading-[22px]">Ask about the pcap data...</p>
        </div>
      </div>
    </div>
  );
}

function Textarea() {
  return (
    <div className="bg-[#0b1326] relative rounded-[4px] shrink-0 w-full" data-name="Textarea">
      <div className="flex flex-row justify-center overflow-clip rounded-[inherit] size-full">
        <div className="content-stretch flex items-start justify-center pb-[35px] pl-[13px] pr-[49px] pt-[13px] relative size-full">
          <Container52 />
        </div>
      </div>
      <div aria-hidden="true" className="absolute border border-[#31394d] border-solid inset-0 pointer-events-none rounded-[4px]" />
    </div>
  );
}

function Container53() {
  return (
    <div className="h-[12px] relative shrink-0 w-[14.25px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 14.25 12">
        <g id="Container">
          <path d={svgPaths.p17041b00} fill="var(--fill-0, #0B1326)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Button15() {
  return (
    <div className="absolute bg-[#00a3ff] bottom-[12px] content-stretch flex items-center justify-center right-[8px] rounded-[4px] size-[32px]" data-name="Button">
      <Container53 />
    </div>
  );
}

function Container51() {
  return (
    <div className="relative shrink-0 w-full" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex flex-col items-start relative size-full">
        <Textarea />
        <Button15 />
      </div>
    </div>
  );
}

function Container56() {
  return (
    <div className="relative shrink-0 size-[10px]" data-name="Container">
      <svg className="absolute block inset-0 size-full" fill="none" preserveAspectRatio="none" viewBox="0 0 10 10">
        <g id="Container">
          <path d={svgPaths.p334ceb10} fill="var(--fill-0, #3F4852)" id="Icon" />
        </g>
      </svg>
    </div>
  );
}

function Container55() {
  return (
    <div className="content-stretch flex gap-[4px] items-center relative shrink-0" data-name="Container">
      <Container56 />
      <div className="flex flex-col font-['Space_Grotesk:Regular',sans-serif] font-normal h-[22px] justify-center leading-[0] relative shrink-0 text-[#3f4852] text-[10px] w-[129.8px]">
        <p className="leading-[22px]">Context: Current Viewpane</p>
      </div>
    </div>
  );
}

function Container57() {
  return (
    <div className="content-stretch flex flex-col items-start relative shrink-0" data-name="Container">
      <div className="flex flex-col font-['Space_Grotesk:Regular',sans-serif] font-normal h-[22px] justify-center leading-[0] relative shrink-0 text-[#3f4852] text-[10px] w-[117.31px]">
        <p className="leading-[22px]">Press Ctrl+Enter to send</p>
      </div>
    </div>
  );
}

function Container54() {
  return (
    <div className="relative shrink-0 w-full" data-name="Container">
      <div className="bg-clip-padding border-0 border-[transparent] border-solid content-stretch flex items-center justify-between relative size-full">
        <Container55 />
        <Container57 />
      </div>
    </div>
  );
}

function InputArea() {
  return (
    <div className="bg-[#131b2e] relative shrink-0 w-full" data-name="Input Area">
      <div aria-hidden="true" className="absolute border-[#31394d] border-solid border-t inset-0 pointer-events-none" />
      <div className="content-stretch flex flex-col gap-[7.5px] items-start pb-[16px] pt-[17px] px-[16px] relative size-full">
        <Container51 />
        <Container54 />
      </div>
    </div>
  );
}

function RightColumnAiAnalysisChat() {
  return (
    <div className="bg-[#060e20] content-stretch flex flex-col h-full items-start relative shrink-0 w-[360px]" data-name="Right Column: AI Analysis Chat (30%)">
      <PanelHeader2 />
      <ChatHistoryArea />
      <InputArea />
    </div>
  );
}

function MainContentArea() {
  return (
    <div className="bg-[#0b1326] content-stretch flex flex-[1_0_0] h-full items-start min-w-px overflow-clip relative z-[1]" data-name="Main Content Area">
      <LeftColumnDataVisualization />
      <RightColumnAiAnalysisChat />
    </div>
  );
}

function MainWorkspaceArea() {
  return (
    <div className="content-stretch flex flex-[1_0_0] isolate items-start min-h-px overflow-clip relative w-full z-[1]" data-name="Main Workspace Area">
      <SideNavBarSharedComponent />
      <MainContentArea />
    </div>
  );
}

export default function HtmlBody() {
  return (
    <div className="content-stretch flex flex-col isolate items-start relative size-full" style={{ backgroundImage: "linear-gradient(90deg, rgb(11, 19, 38) 0%, rgb(11, 19, 38) 100%), linear-gradient(90deg, rgb(255, 255, 255) 0%, rgb(255, 255, 255) 100%)" }} data-name="Html → Body">
      <TopNavBarSharedComponent />
      <MainWorkspaceArea />
    </div>
  );
}