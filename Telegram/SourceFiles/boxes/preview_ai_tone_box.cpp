/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "boxes/preview_ai_tone_box.h"

#include "boxes/create_ai_tone_box.h"
#include "core/ui_integration.h"
#include "data/data_ai_compose_tones.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "ui/controls/custom_emoji_toast_icon.h"
#include "ui/layers/generic_box.h"
#include "ui/layers/show.h"
#include "ui/painter.h"
#include "ui/text/text_entity.h"
#include "ui/text/text_utilities.h"
#include "ui/toast/toast.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/vertical_list.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"

namespace {

constexpr auto kToastDuration = crl::time(4000);

class PreviewAiToneExampleCard final : public Ui::RpWidget {
public:
	explicit PreviewAiToneExampleCard(QWidget *parent);

	void showExample(Data::AiComposeToneExample example);
	[[nodiscard]] rpl::producer<> anotherExampleRequested() const;

protected:
	int resizeGetHeight(int newWidth) override;
	void paintEvent(QPaintEvent *e) override;

private:
	struct Pair {
		not_null<Ui::FlatLabel*> originalTitle;
		not_null<Ui::FlatLabel*> originalBody;
		not_null<Ui::FlatLabel*> resultTitle;
		not_null<Ui::FlatLabel*> resultBody;
		not_null<Ui::LinkButton*> link;
		int innerDividerY = 0;
	};

	std::vector<Pair> _pairs;
	std::vector<int> _pairSeparatorYs;
	rpl::event_stream<> _anotherExampleRequested;

};

PreviewAiToneExampleCard::PreviewAiToneExampleCard(QWidget *parent)
: RpWidget(parent) {
}

void PreviewAiToneExampleCard::showExample(
		Data::AiComposeToneExample example) {
	const auto originalTitle = Ui::CreateChild<Ui::FlatLabel>(
		this,
		tr::lng_ai_compose_original(tr::now),
		st::aiTonePreviewExampleSectionTitle);
	const auto link = Ui::CreateChild<Ui::LinkButton>(
		this,
		tr::lng_ai_compose_tone_preview_add_example(tr::now),
		st::defaultLinkButton);
	rpl::combine(
		this->widthValue(),
		originalTitle->geometryValue(),
		link->widthValue()
	) | rpl::on_next([=](int width, QRect titleGeometry, int linkWidth) {
		const auto right = st::aiTonePreviewExampleCardPadding.left();
		link->moveToRight(right, titleGeometry.top(), width);
	}, lifetime());
	link->clicks()
		| rpl::to_empty
		| rpl::start_to_stream(_anotherExampleRequested, link->lifetime());
	const auto originalBody = Ui::CreateChild<Ui::FlatLabel>(
		this,
		example.from,
		st::aiTonePreviewExampleBody);
	const auto resultTitle = Ui::CreateChild<Ui::FlatLabel>(
		this,
		tr::lng_ai_compose_result(tr::now),
		st::aiTonePreviewExampleSectionTitle);
	const auto resultBody = Ui::CreateChild<Ui::FlatLabel>(
		this,
		example.to,
		st::aiTonePreviewExampleBody);
	originalBody->setSelectable(true);
	resultBody->setSelectable(true);
	originalTitle->show();
	originalBody->show();
	resultTitle->show();
	resultBody->show();
	link->show();
	for (const auto &pair : base::take(_pairs)) {
		delete pair.originalTitle;
		delete pair.originalBody;
		delete pair.resultTitle;
		delete pair.resultBody;
		delete pair.link;
	}
	_pairs.push_back(Pair{
		.originalTitle = originalTitle,
		.originalBody = originalBody,
		.resultTitle = resultTitle,
		.resultBody = resultBody,
		.link = link,
	});
	if (width() > 0) {
		resizeToWidth(width());
	}
}

rpl::producer<> PreviewAiToneExampleCard::anotherExampleRequested() const {
	return _anotherExampleRequested.events();
}

int PreviewAiToneExampleCard::resizeGetHeight(int newWidth) {
	const auto padding = st::aiTonePreviewExampleCardPadding;
	const auto innerLeft = padding.left();
	const auto innerWidth = newWidth - padding.left() - padding.right();
	if (innerWidth <= 0) {
		return 0;
	}
	auto y = padding.top();
	_pairSeparatorYs.clear();
	const auto sectionSpacing = st::aiTonePreviewExampleSectionSpacing;
	const auto sectionSkip = st::aiTonePreviewExampleCardSectionSkip;
	const auto pairSpacing = st::aiTonePreviewExampleCardPairSpacing;
	for (auto i = 0, count = int(_pairs.size()); i != count; ++i) {
		auto &pair = _pairs[i];
		pair.originalTitle->resizeToWidth(innerWidth);
		pair.originalTitle->moveToLeft(innerLeft, y, newWidth);
		y += pair.originalTitle->height() + sectionSpacing;

		pair.originalBody->resizeToWidth(innerWidth);
		pair.originalBody->moveToLeft(innerLeft, y, newWidth);
		y += pair.originalBody->height();

		pair.innerDividerY = y + sectionSkip / 2;
		y += sectionSkip;

		pair.resultTitle->resizeToWidth(innerWidth);
		pair.resultTitle->moveToLeft(innerLeft, y, newWidth);
		y += pair.resultTitle->height() + sectionSpacing;

		pair.resultBody->resizeToWidth(innerWidth);
		pair.resultBody->moveToLeft(innerLeft, y, newWidth);
		y += pair.resultBody->height();

		if (i + 1 != count) {
			_pairSeparatorYs.push_back(y + pairSpacing / 2);
			y += pairSpacing;
		}
	}
	return y + padding.bottom();
}

void PreviewAiToneExampleCard::paintEvent(QPaintEvent *e) {
	auto p = QPainter(this);
	auto hq = PainterHighQualityEnabler(p);
	p.setPen(Qt::NoPen);
	p.setBrush(st::aiTonePreviewExampleCardBg);
	p.drawRoundedRect(
		rect(),
		st::aiTonePreviewExampleCardRadius,
		st::aiTonePreviewExampleCardRadius);

	const auto padding = st::aiTonePreviewExampleCardPadding;
	const auto x1 = padding.left();
	const auto x2 = width() - padding.right();
	p.setBrush(Qt::NoBrush);
	p.setPen(st::aiTonePreviewExampleCardDivider);
	for (const auto &pair : _pairs) {
		p.drawLine(x1, pair.innerDividerY, x2, pair.innerDividerY);
	}
	for (const auto y : _pairSeparatorYs) {
		p.drawLine(x1, y, x2, y);
	}
}

void ShowToneAddedToast(
		std::shared_ptr<Ui::Show> show,
		not_null<Main::Session*> session,
		const Data::AiComposeTone &tone) {
	const auto size = QSize(
		st::aiComposeToneToastIconSize.width(),
		st::aiComposeToneToastIconSize.height());
	show->showToast(Ui::Toast::Config{
		.title = tr::lng_ai_compose_tone_added(tr::now),
		.text = tr::lng_ai_compose_tone_added_description(
			tr::now,
			lt_name,
			tr::marked(tone.title),
			tr::marked),
		.iconContent = Ui::MakeCustomEmojiToastIcon(
			session,
			tone.emojiId,
			size),
		.iconPadding = st::aiComposeToneToastIconPadding,
		.duration = kToastDuration,
	});
}

} // namespace

void PreviewAiToneBox(
		not_null<Ui::GenericBox*> box,
		not_null<Main::Session*> session,
		Data::AiComposeTone tone) {
	box->setStyle(st::aiComposeBox);
	box->setNoContentMargin(true);
	box->setWidth(st::boxWideWidth);
	box->addTopButton(st::aiComposeBoxClose, [=] { box->closeBox(); });

	const auto top = box->setPinnedToTopContent(
		object_ptr<Ui::VerticalLayout>(box));
	Ui::AddSkip(top, st::defaultVerticalListSkip * 2);
	AddAiToneIconPreview(top, session, rpl::single(tone.emojiId), nullptr);
	top->add(
		object_ptr<Ui::FlatLabel>(
			top,
			rpl::single(tone.title),
			st::aiTonePreviewTitleLabel),
		st::aiTonePreviewTitleMargin,
		style::al_top);
	top->add(
		object_ptr<Ui::FlatLabel>(
			top,
			tr::lng_ai_compose_tone_preview_about(),
			st::aiTonePreviewAboutLabel),
		st::aiTonePreviewAboutMargin,
		style::al_top
	)->setTryMakeSimilarLines(true);

	const auto body = box->verticalLayout();

	struct State {
		int examplesCount = 0;
		bool requesting = false;
	};
	const auto state = box->lifetime().make_state<State>();
	state->examplesCount = tone.firstExample ? 1 : 0;

	const auto card = body->add(
		object_ptr<PreviewAiToneExampleCard>(body),
		st::aiTonePreviewExampleCardMargin);
	const auto loadAnother = [=] {
		if (state->requesting) {
			return;
		}
		state->requesting = true;
		const auto num = state->examplesCount;
		session->data().aiComposeTones().getToneExample(
			tone,
			num,
			crl::guard(box, [=](Data::AiComposeToneExample example) {
				state->requesting = false;
				++state->examplesCount;
				card->showExample(std::move(example));
			}),
			crl::guard(box, [=](const MTP::Error &) {
				state->requesting = false;
				box->showToast(tr::lng_ai_compose_error(tr::now));
			}));
	};
	card->anotherExampleRequested(
	) | rpl::on_next(loadAnother, card->lifetime());

	if (tone.firstExample) {
		card->showExample(*tone.firstExample);
	} else {
		loadAnother();
	}

	const auto attribution = body->add(
		object_ptr<Ui::FlatLabel>(body, st::aiTonePreviewAttributionLabel),
		st::aiTonePreviewAttributionMargin,
		style::al_top);

	auto text = tr::marked();
	if (tone.installsCount > 0) {
		text = tr::lng_ai_compose_tone_preview_used_by(
			tr::now,
			lt_count,
			tone.installsCount,
			tr::marked);
	}
	if (const auto user = session->data().userLoaded(tone.authorId)) {
		const auto name = user->shortName();
		auto mention = tr::marked(name);
		mention.entities.push_back(EntityInText(
			EntityType::MentionName,
			0,
			name.size(),
			TextUtilities::MentionNameDataFromFields({
				.selfId = session->userId().bare,
				.userId = tone.authorId.bare,
				.accessHash = user->accessHash(),
			})));
		auto createdBy = tr::lng_ai_compose_tone_preview_created_by(
			tr::now,
			lt_user,
			std::move(mention),
			tr::marked);
		if (!text.empty()) {
			text.append(' ').append(std::move(createdBy));
		} else {
			text = std::move(createdBy);
		}
	}
	if (text.empty()) {
		attribution->setVisible(false);
	} else {
		attribution->setMarkedText(
			std::move(text),
			Core::TextContext({ .session = session }));
	}

	const auto add = box->addButton(
		tr::lng_ai_compose_tone_preview_add(),
		[=] {
			session->data().aiComposeTones().save(tone, false);
			const auto show = box->uiShow();
			box->closeBox();
			ShowToneAddedToast(show, session, tone);
		});
	add->setFullRadius(true);
}
